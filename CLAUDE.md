# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

A C++23 game/engine project built with CMake + Conan and tested with GoogleTest/CTest, developed inside VS Code Dev Containers (GNU, LLVM, MinGW) for a fully reproducible toolchain. Read [`README.md`](README.md) for the full picture, [`REQUIREMENTS.md`](REQUIREMENTS.md) for the MoSCoW-phrased requirements behind the design, and **[`docs/STYLE_GUIDE.md`](docs/STYLE_GUIDE.md) for the full coding style** (naming, formatting, includes, error handling, testing conventions, CMake conventions) — it is authoritative and detailed; don't restate it here, follow it.

## Commands

Run from the repo root, inside a dev container (GNU/LLVM/MinGW), with `CONAN_PROFILE` set to a profile under `profiles/build/` and `profiles/host/` (e.g. `gcc-linux-x86_64`, `clang-linux-x86_64`, `mingw-windows-x86_64`):

```sh
conan install . -of build \
  -pr:b=./profiles/build/${CONAN_PROFILE} \
  -pr:h=./profiles/host/${CONAN_PROFILE} \
  --build=missing -s build_type=Release --lockfile=conan.lock

cmake --preset conan-release
cmake --build build -j24
ctest --test-dir build --output-on-failure
```

Or in VS Code: `Ctrl+Shift+B` runs the same sequence as the default build task (see [`.vscode/tasks.json`](.vscode/tasks.json)).

**Choosing a graphics backend** (`null`, `sdl3` or `raylib`; the default `null` needs no graphics framework and draws nothing):

```sh
conan install . -of build-sdl3 -o gfx_backend=sdl3 \
  -c tools.cmake.cmake_layout:build_folder_vars="['options.gfx_backend']" \
  -pr:b=./profiles/build/${CONAN_PROFILE} \
  -pr:h=./profiles/host/${CONAN_PROFILE} \
  --build=missing -s build_type=Release --lockfile=conan-sdl3.lock

cmake --preset conan-gfx_backend_sdl3-release
cmake --build build-sdl3 -j24
ctest --test-dir build-sdl3 --output-on-failure
```

This assumes the default build above has been installed at least once.
[`CMakePresets.json`](CMakePresets.json) includes `build/CMakePresets.json` unconditionally, and its `conan-coverage` preset inherits the `conan-release` one generated there, so CMake refuses to read *any* preset until the default configuration exists.
On a fresh clone, run the `conan install . -of build` above first.

The Conan option sets the `ANTWIKA_GFX_BACKEND` CMake variable, which names a directory under [`backends/`](backends/); an unknown value fails at configure time with the list of ones that exist.
The `build_folder_vars` conf is what puts the backend in the preset name, so the sdl3 configuration does not collide with the default build's `conan-release` preset.
Each configuration has its own lockfile, because selecting a backend changes the dependency graph.

**Updating the lockfiles** after a dependency bump in `conanfile.py` (what Renovate leaves stale):

```sh
scripts/update_lockfiles.sh   # or: Tasks: Run Task > Update Conan lockfiles
```

It re-resolves every lockfile from scratch against every profile under `profiles/host/`, since CI builds all of them against the same files.

Set `SDL_VIDEODRIVER=dummy` to run the SDL build with no display, or use `xvfb-run` for any backend, which is how the conformance suite is exercised without a desktop session.
`raylib` reports `maxWindows() == 1`, since it keeps its one window in global state; the conformance suite skips its multi-window tests for such a backend rather than failing them.

**Run a single test binary / test case:**

```sh
ctest --test-dir build -R antwika_replay_tests --output-on-failure
build/bin/antwika_replay_tests --gtest_filter='ReplayReaderTest.*'
```

**Run the apps:**

```sh
build/bin/antwika_game --record demo.replay   # or --replay demo.replay
build/bin/antwika_life --record demo.replay
build/bin/antwika_task_worker --record demo.replay
build/bin/antwika_poker --record demo.replay
build/bin/antwika_sudoku [--puzzle my-puzzle.txt]
```

`antwika_poker` also opens a window and draws the table each tick.
`--tick-delay-ms <n>` holds each frame for `n` ms and keeps the final frame up until the window is closed; it defaults to 0, which is what keeps the default terminal run unchanged and stops the `null` backend (which never reports a close) from wedging it.
A real backend needs a display, so use `SDL_VIDEODRIVER=dummy` or `xvfb-run` without one.

**Coverage build** (separate `build-coverage/` dir, GNU/LLVM only — not MinGW):

```sh
cmake --preset conan-coverage
cmake --build build-coverage -j24
ctest --test-dir build-coverage
gcovr --root . --filter 'src/.*' --exclude '.*/tests/.*' --print-summary build-coverage
```

CI requires 100% line/function/branch coverage on the GNU leg (`scripts/check_full_coverage.py`); see [`docs/confirming-unreachable-branches.md`](docs/confirming-unreachable-branches.md) before excluding any line with `GCOVR_EXCL_LINE`.

**Checker scripts** (all enforced in CI, all runnable locally the same way):

```sh
python3 scripts/check_unused_test_doubles.py     # every mock/fake is used
python3 scripts/check_one_sentence_per_line.py   # comments/markdown prose
python3 scripts/check_line_length.py             # 80-char limit, src/ + scripts/
```

Each checker script has its own test under `scripts/tests/`, run with e.g. `python3 scripts/tests/test_check_line_length.py`.

## Architecture

The system is layered as small, single-purpose libraries under `src/libs/`, composed by apps under `src/apps/`. Each module (lib or app) owns its own `CMakeLists.txt`, `include/`, `src/`, and `tests/`, and builds a `antwika_<module>` target aliased to `antwika::<module>`.

**The tick loop and replay determinism** (see [`blog/001-building-a-deterministic-replay-system.md`](blog/001-building-a-deterministic-replay-system.md)):

- `antwika::engine::IEngine` / `Engine` steps one fixed tick at a time; it is domain-agnostic and knows nothing about any app's state.
- `antwika::event::Event` is the one, uniform event mechanism — app code defines its own event kinds (e.g. `game.score_increment`, `life.toggle_cell`, `task.submit`) on top of the engine's built-in `engine.tick`, dot-namespaced, with no special-casing between "built-in" and app-defined events.
- `antwika::replay::EngineLoop` is the one code path shared by live and replay runs: each tick it asks an `IReplaySource` for that tick's events, dispatches them through a `TickedEventDispatcher`, then steps the engine. Live vs. replay differ **only** in what implements `IReplaySource` — a script feeding `ReplaySource` from a loaded file, vs. a live/hand-authored input source. This is what makes replay reproduce state by construction, not by convention.
- `antwika::replay` reads/writes replays as JSON (`ReplayReader`/`ReplayWriter`, backed by `EventJson`/`PayloadJson`/`EventSchema` and validated with `json-schema-validator`). Only externally-supplied events are persisted — anything the engine regenerates deterministically on its own (like `engine.tick`) is never stored in the replay.
- `ReplayFormatError` / `EngineLoopError` are the specific exception types for bad replay input and loop misuse respectively, following the project's one-exception-type-per-failure-category rule.
- `antwika::replay::ReplayCli` parses the `--record <path>` / `--replay <path>` flags shared by every app's `main.cpp`.

**Application state**: each app owns its state and how events mutate it — the engine has no opinion here.

- `apps/game` holds state as a plain `GameState` struct, mutated by `GameStateReducer` (an `antwika::reducer::IReducer` implementation) reacting to tick-stamped events through `ITickEventSink`.
- `apps/life` (Conway's Game of Life) holds state in an `antwika::ecs::World` instead: each cell is an entity with a `Cell` component, and a single `LifeSystem` advances every cell one generation per tick via the double-buffered `World`/`SystemScheduler` — see [`blog/003-an-entity-component-system-with-nowhere-to-hide-a-mutation.md`](blog/003-an-entity-component-system-with-nowhere-to-hide-a-mutation.md). Cells are toggled via a `life.toggle_cell` event.
- `apps/task_worker` combines `antwika::ecs` with `antwika::scheduler`: a fixed pool of `Worker` entities pulls tasks off a deterministic, priority-ordered, budget-bounded `Scheduler`. `task.submit` events (parsed `id,priority,durationTicks,label[,dependsOnId]`) are scheduled by `TaskSubmissionSink`; `TaskDispatchSystem` runs the scheduler once per tick with that tick's idle-worker count as its budget, so dispatch never exceeds free workers. Dependency cycles are unreachable by construction (id-ordering), not by a runtime check — see [`blog/006-a-job-scheduler-and-a-worker-pool-that-cant-lie-to-itself.md`](blog/006-a-job-scheduler-and-a-worker-pool-that-cant-lie-to-itself.md).
- `apps/poker` is a no-limit Texas hold'em cash game on top of a new `antwika::holdem` library.
One engine tick is one step of the poker loop: a deal, or one player being asked to act, through `holdem::TableRunner` and `holdem::IAgent`.
`holdem::Table` owns the betting rules and stage progression (pre-flop, flop, turn, river, showdown) and pays out via side pots; `holdem::evaluate()` scores 5-7 cards into a single comparable `HandValue` using only shifts, ands and ors over four per-suit 13-bit rank masks -- greater is stronger, equal is a split pot.
The app tracks balances outside the games in `poker::BankrollLedger`, and `poker::CashGame` is the only path between a bankroll and a seat, so a buy-in can never exceed what a player holds.
Only `poker.deposit`/`poker.buy_in`/`poker.cash_out` are persisted: the shuffle is seeded from `RoomConfig` and `poker::PolicyAgent` is a pure function of the `TableView` it is handed, so cards and decisions are regenerated rather than recorded -- see [`blog/010-a-poker-hand-in-one-number.md`](blog/010-a-poker-hand-in-one-number.md).
`poker::TablePrinter` writes every hand out in the standard hand-history layout, deriving the blinds, the raise sizes and the uncalled bet from `holdem::StepOutcome` rather than recomputing the betting -- see [`blog/011-writing-a-hand-history-the-rest-of-the-world-can-read.md`](blog/011-writing-a-hand-history-the-rest-of-the-world-can-read.md).
The same session draws itself through `antwika::gfx`, split so that rendering is write-only in structure rather than by promise: `poker::snapshotOf()` takes an immutable `poker::TableSnapshot` (the spectator's answer to `holdem::TableView`), `poker::TableScene` turns that into drawing calls, and `poker::TableRenderSink` runs it once per `engine.tick` -- registered *after* `PokerRoomSink`, since that is what steps the table.
The only route back in is `poker::WindowCloseSource`, an `IReplaySource` decorator that appends `engine.stop` once the window has gone, so a close is ordinary replay input and lands in a `--record` file like anything else.
- `apps/sudoku` is unrelated to the tick/replay system: it's a showcase for `antwika::wfc` (Wave Function Collapse) — a standalone, dependency-free, deterministic constraint solver operating on a flat, index-addressed `std::vector` of cells with geometry expressed entirely through `IConstraint`s (no grid concept inside the library). `apps/sudoku` expresses the 81-cell puzzle and its row/column/box rules as `AllDifferentConstraint`s over that flat array — see [`blog/005-wave-function-collapse-that-never-guesses.md`](blog/005-wave-function-collapse-that-never-guesses.md).

**Supporting libs**: `antwika::time` (fixed-tick `Tick` type, `IClock`/`SystemClock`) and `antwika::log` (`ILogger`/`Logger`, `IAppender`/`IFormatter`/`ILogPolicy` — composable logging with no global state) are used across apps but carry no tick/replay logic of their own.

`antwika::gfx` abstracts opening and rendering to windows (`IGfxBackend`/`IWindow`/`IRenderer`, `GfxError`), so no code under `src/` names a concrete graphics framework — SDL, raylib and friends arrive as statically linked backends under `backends/`, chosen at build time. Rendering is a write-only projection of state and never feeds back into the tick loop, so replays stay reproducible under the headless `NullBackend`. See [`blog/012-a-window-that-cant-talk-back.md`](blog/012-a-window-that-cant-talk-back.md) for how an app hangs rendering off the tick loop without letting it feed back in.

Textures are decoded once and uploaded per backend: `gfx::PngReader::read()` turns a byte stream into a `gfx::Bitmap` of straight RGBA (stb_image, compiled `STB_IMAGE_STATIC` in one TU because raylib links its own copy), `IRenderer::createTexture()` uploads it, and `drawTexture(texture, source, destination, tint)` blits part of it with a colour and alpha modulation.
The library opens no files — an app does that, as `apps/gfx_demo` does with the PNG path baked in at configure time.
A texture belongs to the renderer that made it: drawing it through any other draws nothing, and it may safely outlive its window, because each renderer's `detach()` frees its live textures before the framework tears the device down.
Write-only still holds — `ITexture` is opaque, and there is no pixel read-back, render target or screenshot anywhere in the interface.

`antwika::ui` is an immediate-mode UI library on top of `antwika::gfx`: nestable row/column/panel layouts, labels and buttons, drawn through `IRenderer`'s rectangle and text calls and laid out arithmetically from `gfx::textSize()` alone, so it asks no backend to measure anything. It depends on `antwika::gfx` and nothing else — not `event`, not `replay`, not `input`.
The caller writes immediate-mode code, but what that code builds is a flat node arena (`src/LayoutTree.hpp`, private), laid out only when `Context::finish()` is called. That deferral is the whole design: a container cannot size itself from children it has not declared yet, so a one-pass immediate-mode layout can only nest when the caller has already computed every number. Because a child is always appended after its parent, measuring is one descending index loop and arranging one ascending one — flat loops, not recursion, so there is no nesting depth to exceed, and ascending order is also correct paint order for a renderer with no z-order.
`row()`/`column()`/`panel()` return a `[[nodiscard]] ui::Scope` that closes the container in its destructor, and `Context` has no `end()` of any kind, so a mis-nested layout is not expressible rather than checked. `finish()` returns a `ui::DrawList` — a plain vector of `FillRect`/`DrawText` values — and `ui::paint()` is the only thing in the library that touches `IRenderer`. Keeping the picture as a value is what lets a whole layout be asserted with `EXPECT_EQ` and no mock; `paint()` never clears and never presents, since a UI is drawn over what is already there.
Scope is deliberately display-only: nothing reads a pointer or keyboard and nothing reaches the engine, so `ButtonState` is supplied by the caller rather than worked out. There is no clipping (`IRenderer` has no scissor), so containment is the layout's job — a container with too little room shrinks its children in proportion rather than letting them escape. `apps/gfx_demo` is the showcase: a panel painted over the bars and the logo, last, so it reads as being in front of them.

## Notes for AI agents

- **Always work in a separate git worktree, never directly in the primary checkout.** Before making any change, create/enter a dedicated worktree for the task (e.g. `git worktree add ../antwika-<task> -b <task>`), do all editing, building, and testing there, and only merge back when the work is done. This keeps `main` clean and lets several tasks build in parallel without clobbering each other's `build/` directory.
- The blog posts under `blog/` are design write-ups for *why* a piece was built the way it was, written after the fact — read the relevant one before changing a library's core abstraction, since it usually explains a constraint that isn't obvious from the code alone.
- Prefer running a single test binary (or `--gtest_filter`) over the full `ctest` suite while iterating; run the full suite before considering a change done.
