# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

A C++23 game/engine project built with CMake + Conan and tested with GoogleTest/CTest, developed inside VS Code Dev Containers (GNU, LLVM, MinGW) for a fully reproducible toolchain.
Read [`README.md`](README.md) for the full picture, [`REQUIREMENTS.md`](REQUIREMENTS.md) for the MoSCoW-phrased requirements behind the design, and **[`docs/STYLE_GUIDE.md`](docs/STYLE_GUIDE.md) for the full coding style** (naming, formatting, includes, error handling, testing conventions, CMake conventions) — it is authoritative and detailed; don't restate it here, follow it.
`docs/` holds only documents that are still normative; a plan document moves to [`docs/history/`](docs/history/) once the work it describes has shipped, and each one says at the top what superseded it, so a listing of `docs/` tells you what is current without opening anything.

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

**Choosing a graphics and input backend** (`null`, `sdl3` or `raylib`; the default `null` needs no framework, draws nothing and reports no input):

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

This assumes the default build above has been installed at least once. [`CMakePresets.json`](CMakePresets.json) includes `build/CMakePresets.json` unconditionally, and its `conan-coverage` preset inherits the `conan-release` one generated there, so CMake refuses to read *any* preset until the default configuration exists.
On a fresh clone, run the `conan install . -of build` above first.

The Conan option sets the `ANTWIKA_GFX_BACKEND` CMake variable, which names a directory under [`backends/`](backends/); an unknown value fails at configure time with the list of ones that exist.
The `build_folder_vars` conf is what puts the backend in the preset name, so the sdl3 configuration does not collide with the default build's `conan-release` preset.
Each configuration has its own lockfile, because selecting a backend changes the dependency graph.

Input has its own selection, `-o input_backend=` and the `ANTWIKA_INPUT_BACKEND` CMake variable, defaulting to `auto`, which resolves to whatever `gfx_backend` is.
So the command above gets sdl3 keyboard and mouse for free, and the default build stays `null`/`null` with no new dependency.
Setting the two apart is legal for input with no window (`-o gfx_backend=null -o input_backend=sdl3`) or a window with no input.
Naming two *different* real frameworks is refused, by `validate()` in [`conanfile.py`](conanfile.py) and again in [`backends/CMakeLists.txt`](backends/CMakeLists.txt): they would fight over one process-global event queue, and whichever polled second would silently lose events.
A directory selected for one subsystem only builds that subsystem's target, which is why each `backends/<name>/CMakeLists.txt` guards its two targets separately.

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
build/bin/antwika_game                        # empty grid, runs until quit
build/bin/antwika_game --record demo.replay   # or --replay demo.replay
build/bin/antwika_life                        # runs until stopped
build/bin/antwika_life --record demo.replay
build/bin/antwika_task_worker --record demo.replay
build/bin/antwika_poker --record demo.replay
build/bin/antwika_sudoku [--puzzle my-puzzle.txt]
```

`antwika_life` opens a window, draws the board each tick, and takes mouse input.
It has no end of its own: it runs until the window is closed, or until a replay dispatches `engine.stop`.
A headless build reports neither, so `Ctrl+C` is what ends one -- and a `--record` run only writes its file once the run ends.
Both the windowed and the headless run are paced through `TickPacer`, since a run that never ends would otherwise go flat out.

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

The system is layered as small, single-purpose libraries under `src/libs/`, composed by apps under `src/apps/`.
Each module (lib or app) owns its own `CMakeLists.txt`, `include/`, `src/`, and `tests/`, and builds a `antwika_<module>` target aliased to `antwika::<module>`.

**The tick loop and replay determinism** (see [`blog/001-building-a-deterministic-replay-system.md`](blog/001-building-a-deterministic-replay-system.md)):

- `antwika::engine::IEngine` / `Engine` steps one fixed tick at a time; it is domain-agnostic and knows nothing about any app's state.
- `antwika::event::Event` is the one, uniform event mechanism — app code defines its own event kinds (e.g. `game.score_increment`, `life.toggle_cell`, `task.submit`) on top of the engine's built-in `engine.tick`, dot-namespaced, with no special-casing between "built-in" and app-defined events.
- `antwika::replay::EngineLoop` is the one code path shared by live and replay runs: each tick it asks an `IReplaySource` for that tick's events, dispatches them through a `TickedEventDispatcher`, then steps the engine.
  Live vs. replay differ **only** in what implements `IReplaySource` — a script feeding `ReplaySource` from a loaded file, vs. a live/hand-authored input source.
  This is what makes replay reproduce state by construction, not by convention.
- `antwika::replay` reads/writes replays as JSON (`ReplayReader`/`ReplayWriter`, backed by `EventJson`/`PayloadJson`/`EventSchema` and validated with `json-schema-validator`).
  Only externally-supplied events are persisted — anything the engine regenerates deterministically on its own (like `engine.tick`) is never stored in the replay.
  `ReplayWriter::Layout` chooses whether a document is indented: `saveReplayFile()` writes `Compact`, since a `--record` run is read by `ReplayReader` rather than by a person, while the demo replays checked in under `src/apps/*/replays/` stay `Pretty` and diffable.
- `ReplayFormatError` / `EngineLoopError` are the specific exception types for bad replay input and loop misuse respectively, following the project's one-exception-type-per-failure-category rule.
- `antwika::replay::ReplayCli` parses the `--record <path>` / `--replay <path>` flags shared by every app's `main.cpp`.

**Application state**: each app owns its state and how events mutate it — the engine has no opinion here.

- `apps/game` is an isometric grid you build on with the mouse: left-click lays a path tile, right-click drops a walker onto one, middle-drag pans and the wheel zooms.
  Walkers advance one cell per tick along the paths, preferring a right turn at an intersection and reversing at a dead end -- both of which fall out of one preference order in `game::nextFacing()` rather than two rules.
  The plain `GameState` struct and its `GameStateReducer` are still there, folding `game.score_increment` alongside the grid.
  **The camera is simulation state, not render state**, which is the load-bearing decision: a click arrives as a pixel, and which cell it means depends entirely on the camera, so a renderer-owned camera would leave a replay resolving recorded clicks against a different view.
  That is also why zoom is an index into a table of whole tile sizes rather than a scale factor, why `game::floorDiv()` exists instead of `operator/`, and why the projection is anchored to the camera's pan rather than the canvas centre -- anchoring to the centre would make a window resize change which cell a pixel means.
  **The app defines no event for placing anything**: a click is the input, `game::GridSink` turns it into a placement inside the tick path, and the replay stores the click and regenerates the placement -- persisting both would lay two tiles per click.
  Every tile is one blit from one texture atlas (`src/apps/game/assets/atlas.png`), addressed through `game::TileAtlas.hpp`: `game::GridScene` draws no shape of its own, so the lattice is painted into the ground tile's own edges rather than being lines the scene places, and a junction is one of sixteen road tiles indexed by which neighbours the cell joins.
  That mask is worked out in the scene from the snapshot's paths, which arrive in ascending order, so a neighbour is a binary search rather than a second index to keep in step -- and it stays out of `SceneSnapshot` and `GameSummary`, since which tile a road shows is a picture, not state a replay has to reproduce.
  The picture itself is generated by [`scripts/generate_game_atlas.py`](scripts/generate_game_atlas.py) from the same slot numbers `TileAtlas.hpp` addresses it with, and its art is drawn in grid space so that a road stub's shape falls out of the same projection the game blits it through; CI fails if the committed PNG has drifted from the generator.
  A toolbar of zoom and reset-view buttons is drawn over the grid by `game::Toolbar`, described and resolved once per tick by `game::UiSink` -- registered *before* `GridSink`, so a press is resolved against the bar before the grid sees it -- and painted last by `RenderSystem`.
  It defines no event either, for the same reason: what a recording holds is the click.
  `game::UiOverlay` is the one fact the three share, and it owns the canvas the bar is laid out against (the size the window was *asked* for) so nothing can lay it out against one size and hit-test it against another.
  What the bar covers, it covers from the grid too: `GridSink` skips a press or a scroll the overlay reports as covered, though not a movement, so a pan begun on the grid carries on across the bar.
  **A button here lights up on the press rather than on approach**, and that is `input::IdleMotionSource` in this app's chain rather than anything `antwika::ui` decides: idle pointer movement is held back until something reads it, so a hover appearance updates only when a button, a wheel or a key arrives.
  Clicking is unaffected, since the gate releases the latched movement ahead of the press and a press carries its own position.
  Taking the gate out of `main.cpp` would buy live hover back at the recording size it was added to save, which is the trade to weigh if that ever matters more.
  It starts on an empty grid and loads nothing unless `--replay` says so, so what a session contains is what somebody clicked.
  It runs until Escape is pressed or the window is closed -- both of which are input, so both are recorded and both replay.
  Neither reaches the `null` backend, so that build runs until interrupted, and a `--record` there never gets to save.
  `src/apps/game/replays/demo.json` is a sample session to pass to `--replay`.
  See [`blog/013-the-camera-is-simulation-state.md`](blog/013-the-camera-is-simulation-state.md).
- `apps/life` (Conway's Game of Life) holds state in an `antwika::ecs::World` instead: each cell is an entity with a `Cell` component, and a single `LifeSystem` advances every cell one generation per tick via the double-buffered `World`/`SystemScheduler` — see [`blog/003-an-entity-component-system-with-nowhere-to-hide-a-mutation.md`](blog/003-an-entity-component-system-with-nowhere-to-hide-a-mutation.md).
  Cells are toggled either by a scripted `life.toggle_cell` event or by dragging over them with the mouse.
  The drag is `antwika::input`'s side: `input::LiveInputSource` puts each edge into the tick stream, and `life::PointerToggleSink` decodes the `input.pointer_*` events and toggles the cell under the pointer — so a `--record` run persists the click and regenerates the toggle, per the rule that a replay holds only external input.
  Where a cell is drawn and which cell a click lands in are one function, `life::layoutFor()`/`life::cellAt()` in `BoardLayout.hpp`, shared by `BoardScene` and `PointerToggleSink` so the two cannot drift.
  That mapping is against the *configured* window size rather than the size a window reports, and the window is not resizable, which is what keeps a recorded session landing on the same cells under a different backend.
  `PointerToggleSink` toggles a cell at most once per drag, and keeps its own note of what the current tick staged, because `World` hands out the committed board and would otherwise let two drags over one cell in a tick collapse into one toggle.
  Holding the button pauses the generations: the sink reports the drag through a shared `life::DragState`, and `life::DragPausedSystem` wraps `LifeSystem` and stages nothing while a drag is under way.
  Only that one system stops — the tick, the commit and every observer still run, so the cells being drawn appear immediately, which is the whole point of pausing.
  A press that lands off the board still pauses, since what pauses is holding the button rather than hitting a cell.
  Which ticks were paused follows from the recorded presses and releases, so a replay pauses on exactly the same ones.
  The run is uncapped: only closing the window (via `WindowInputSource`) or a replay's `engine.stop` ends it.
- `apps/task_worker` combines `antwika::ecs` with `antwika::scheduler`: a fixed pool of `Worker` entities pulls tasks off a deterministic, priority-ordered, budget-bounded `Scheduler`.
  `task.submit` events (parsed `id,priority,durationTicks,label[,dependsOnId]`) are scheduled by `TaskSubmissionSink`; `TaskDispatchSystem` runs the scheduler once per tick with that tick's idle-worker count as its budget, so dispatch never exceeds free workers.
  Dependency cycles are unreachable by construction (id-ordering), not by a runtime check — see [`blog/006-a-job-scheduler-and-a-worker-pool-that-cant-lie-to-itself.md`](blog/006-a-job-scheduler-and-a-worker-pool-that-cant-lie-to-itself.md).
- `apps/poker` is a no-limit Texas hold'em cash game on top of a new `antwika::holdem` library.
  One engine tick is one step of the poker loop: a deal, or one player being asked to act, through `holdem::TableRunner` and `holdem::IAgent`.
  `holdem::Table` owns the betting rules and stage progression (pre-flop, flop, turn, river, showdown) and pays out via side pots; `holdem::evaluate()` scores 5-7 cards into a single comparable `HandValue` using only shifts, ands and ors over four per-suit 13-bit rank masks -- greater is stronger, equal is a split pot.
  The app tracks balances outside the games in `poker::BankrollLedger`, and `poker::CashGame` is the only path between a bankroll and a seat, so a buy-in can never exceed what a player holds.
  Only `poker.deposit`/`poker.buy_in`/`poker.cash_out` are persisted: the shuffle is seeded from `RoomConfig` and `poker::PolicyAgent` is a pure function of the `TableView` it is handed, so cards and decisions are regenerated rather than recorded -- see [`blog/010-a-poker-hand-in-one-number.md`](blog/010-a-poker-hand-in-one-number.md).
  `poker::TablePrinter` writes every hand out in the standard hand-history layout, deriving the blinds, the raise sizes and the uncalled bet from `holdem::StepOutcome` rather than recomputing the betting -- see [`blog/011-writing-a-hand-history-the-rest-of-the-world-can-read.md`](blog/011-writing-a-hand-history-the-rest-of-the-world-can-read.md).
  The same session draws itself through `antwika::gfx`, split so that rendering is write-only in structure rather than by promise: `poker::snapshotOf()` takes an immutable `poker::TableSnapshot` (the spectator's answer to `holdem::TableView`), `poker::TableScene` turns that into drawing calls, and `poker::TableRenderSink` runs it once per `engine.tick` -- registered *after* `PokerRoomSink`, since that is what steps the table.
  The only route back in is `poker::WindowCloseSource`, an `IReplaySource` decorator that appends `engine.stop` once the window has gone, so a close is ordinary replay input and lands in a `--record` file like anything else.
- `apps/sudoku` is unrelated to the tick/replay system: it's a showcase for `antwika::wfc` (Wave Function Collapse) — a standalone, dependency-free, deterministic constraint solver operating on a flat, index-addressed `std::vector` of cells with geometry expressed entirely through `IConstraint`s (no grid concept inside the library).
  `apps/sudoku` expresses the 81-cell puzzle and its row/column/box rules as `AllDifferentConstraint`s over that flat array — see [`blog/005-wave-function-collapse-that-never-guesses.md`](blog/005-wave-function-collapse-that-never-guesses.md).

**Supporting libs**: `antwika::time` (fixed-tick `Tick` type, `IClock`/`SystemClock`) and `antwika::log` (`ILogger`/`Logger`, `IAppender`/`IFormatter`/`ILogPolicy` — composable logging with no global state) are used across apps but carry no tick/replay logic of their own.

`antwika::input` abstracts reading a keyboard and a pointer (`IInputBackend`/`InputEvent`/`InputCapabilities`, `InputError`), so no code under `src/` names a concrete input framework; backends live under [`backends/`](backends/) beside the graphics ones and are chosen at build time by `ANTWIKA_INPUT_BACKEND`.
It deliberately does **not** depend on `antwika::gfx`, and `antwika::gfx` does not depend on it — reading input does not require opening a window, which is why `input::Position` duplicates `gfx::Point` rather than reusing it, and why an input event does not say which window it arrived at.
**That rule is about the source, not the link line**: no file under `src/libs/input` includes a `<antwika/gfx/...>` header or names a `gfx::` type, and that is what it forbids.
`antwika::gfx` is nevertheless *on* that link line, transitively: `antwika::replay` links `antwika::ecs` and `antwika::gfx` for `TickPacer` and `WindowInputSource`, and `antwika::input` links `antwika::replay` because every source it offers is an `IReplaySource`.
That was reviewed and accepted rather than overlooked — a linker seeing a library is not a dependency in the sense the rule cares about, since nothing in `input` can call into it — so a reader who finds `gfx` in `antwika_input`'s transitive link set has not found a violation.
Every `InputEvent` is an *edge* (a press, a release, a move, a notch) and never a statement of what is currently held, which is what lets a queue-based framework (SDL) and a state-polling one (raylib) implement the same interface.
Live input reaches the engine only through `IReplaySource`, via `LiveInputSource`, and is persisted by `InputEventCodec` as `input.*` events with symbolic key and button names rather than platform scancodes, so a session recorded under one backend replays under another; no `input.*` name may ever be added to an app's `kSelfGeneratedEventNames`.
What a recording holds is thinned by two decorators that sit *upstream* of `TickEventRecorder`, which is the only place a reduction may happen: doing it after the recorder would make the file disagree with the run that wrote it, and doing it in a backend would hide it behind the seam.
`CoalescingPointerSource` keeps only the last of each run of movements inside a tick; `IdleMotionSource` holds back movement that arrives while no button is held, latching the last of it and releasing it immediately ahead of the first event that could read a position -- since `input.pointer_scroll` carries none of its own and a zoom anchors on the folded one.
Which of the two an app may attach is an app-level question: `game` takes both, `life` takes only the gate, because a drag toggles every cell it crosses and thinning a run inside a tick would skip some.
An app attaching the gate cannot draw anything that follows a free-moving pointer, since the movements between clicks are deliberately not in the tick stream -- see [`docs/history/replay-size-plan.md`](docs/history/replay-size-plan.md).
Because SDL drains one process-global queue for windows *and* input, `backends/sdl3` owns a reference-counted `Sdl3Pump` shared by both of its targets, which calls `SDL_PollEvent` once and routes each event into a window queue or an input queue — see [`docs/history/input-plan.md`](docs/history/input-plan.md) for why that sharing belongs in `backends/` rather than in `src/`.
The `raylib` input backend reports a pointer and no keyboard, and synthesises edges by diffing state, since raylib has no queue at all.

`antwika::gfx` abstracts opening and rendering to windows (`IGfxBackend`/`IWindow`/`IRenderer`, `GfxError`), so no code under `src/` names a concrete graphics framework — SDL, raylib and friends arrive as statically linked backends under `backends/`, chosen at build time.
Rendering is a write-only projection of state and never feeds back into the tick loop, so replays stay reproducible under the headless `NullBackend`.
See [`blog/012-a-window-that-cant-talk-back.md`](blog/012-a-window-that-cant-talk-back.md) for how an app hangs rendering off the tick loop without letting it feed back in.

Textures are decoded once and uploaded per backend: `gfx::PngReader::read()` turns a byte stream into a `gfx::Bitmap` of straight RGBA (stb_image, compiled `STB_IMAGE_STATIC` in one TU because raylib links its own copy), `IRenderer::createTexture()` uploads it, and `drawTexture(texture, source, destination, tint)` blits part of it with a colour and alpha modulation.
The library opens no files — an app does that, as `apps/gfx_demo` does with the PNG path baked in at configure time.
A texture belongs to the renderer that made it: drawing it through any other draws nothing, and it may safely outlive its window, because each renderer's `detach()` frees its live textures before the framework tears the device down.
Write-only still holds — `ITexture` is opaque, and there is no pixel read-back, render target or screenshot anywhere in the interface.

`antwika::ui` is an immediate-mode UI library on top of `antwika::gfx`: nestable row/column/panel layouts, labels and buttons, drawn through `IRenderer`'s rectangle and text calls and laid out arithmetically from `gfx::textSize()` alone, so it asks no backend to measure anything.
It depends on `antwika::gfx` and nothing else — not `event`, not `replay`, not `input`.
The caller writes immediate-mode code, but what that code builds is a flat node arena (`src/LayoutTree.hpp`, private), laid out only when `Context::finish()` is called.
That deferral is the whole design: a container cannot size itself from children it has not declared yet, so a one-pass immediate-mode layout can only nest when the caller has already computed every number.
Because a child is always appended after its parent, measuring is one descending index loop and arranging one ascending one — flat loops, not recursion, so there is no nesting depth to exceed, and ascending order is also correct paint order for a renderer with no z-order.
`row()`/`column()`/`panel()` return a `[[nodiscard]] ui::Scope` that closes the container in its destructor, and `Context` has no `end()` of any kind, so a mis-nested layout is not expressible rather than checked.
`finish()` returns a `ui::Frame` — a `ui::DrawList` of plain `FillRect`/`DrawText` values, plus the `ui::Interactions` the pointer produced — and `ui::paint()` is the only thing in the library that touches `IRenderer`.
Keeping the picture as a value is what lets a whole layout be asserted with `EXPECT_EQ` and no mock; `paint()` never clears and never presents, since a UI is drawn over what is already there.
**A button is clickable, and the library still reads no device.** The pointer arrives as a `ui::Pointer` argument to `Context` (default: no pointer at all, so a display-only caller is unchanged), an application having folded `antwika::input`'s edges into it.
A widget named with a `ui::WidgetId` in its `ui::ButtonSpec` works out its own hovered/pressed appearance, and `Frame::interactions` reports the `hovered`/`activated` id and whether the pointer is over anything the UI filled in.
`ButtonState` is still accepted as an override, for the app that knows which button is in play.
Interaction is one new private stage, `detail::resolve()` (`src/Resolve.hpp`), run between `layout()` and `flatten()`: it hit-tests the arena by *descending* index — ascending is paint order, so descending is front-to-back, and layout's containment guarantee makes the frontmost hit the deepest — then writes each interactive node's background.
Everything is therefore resolved against the same frame's layout, and **nothing is retained between frames**: activation is on the press, deliberately, because a press-then-release match would be cross-frame state a replay has to regenerate.
Ids are symbolic and caller-supplied rather than declaration order, since an id is what crosses back into application state.
There is no clipping (`IRenderer` has no scissor), so containment is the layout's job — a container with too little room shrinks its children in proportion rather than letting them escape.
`apps/gfx_demo` is the showcase: a panel painted over the bars and the logo, last, so it reads as being in front of them, with two buttons that count and reset a click counter the loop owns.
**An app must describe and resolve its UI inside the tick path, downstream of the recorder** — never in a renderer — so a replay stores the click and regenerates which widget it activated; no `ui.*` event name may ever exist.
The canvas it is laid out and hit-tested against must be the configured window size, never the size a window reports, for the reason `life::PointerToggleSink` gives about cells: a hit-test is a function of the layout, and the layout is a function of the canvas.
`apps/game`'s `UiSink`/`UiOverlay` is the worked example.
**Tab, Shift+Tab and Enter reach the same buttons, and the library still reads no device**: key edges arrive as a `ui::Keyboard` value argument to `Context` — a list of symbolic `ui::Key` values in arrival order, defined by `antwika::ui` itself, defaulting to none so an existing caller's output is byte-identical.
Focus is the one thing a keyboard UI needs that outlives a frame, and it is **passed through rather than kept**: last frame's focused `WidgetId` goes *in* as a `Context` argument and this frame's comes back *out* as `Frame::interactions.focused`, so the state lives in application state — where a replay regenerates it from the recorded key presses — and the library stays as stateless as the press-time activation rule requires.
The tab order is the arena's ascending index, which is declaration order, so no second order can drift from the layout; a repeated id is one stop, an unnamed button is none, Tab from nothing takes the first widget and Shift+Tab the last, and both wrap.
A pointer press moves focus to whatever it activated, so the ring and the keystrokes cannot end up on different widgets, and Enter reports through `Interactions::activated` exactly as a press does, so one code path handles both.
The focused widget draws `Theme::focusRing` (yellow) `Theme::focusRingThickness` pixels thick as four `FillRect`s appended *after* every widget, since `IRenderer` has no stroke and a container declared later would otherwise paint over a ring drawn in place.

## Notes for AI agents

- **Always work in a separate git worktree, never directly in the primary checkout.** Before making any change, create/enter a dedicated worktree for the task (`git worktree add .worktrees/<task> -b <task>`), do all editing, building, and testing there, and only merge back when the work is done.
  This keeps `main` clean and lets several tasks build in parallel without clobbering each other's `build/` directory.
  `.worktrees/` is the agreed home for them and `.gitignore` covers it, so a worktree and its build output never show up as untracked state in the primary checkout; `.claude/` and `core.*` are ignored for the same reason.
- The blog posts under `blog/` are design write-ups for *why* a piece was built the way it was, written after the fact — read the relevant one before changing a library's core abstraction, since it usually explains a constraint that isn't obvious from the code alone.
- Prefer running a single test binary (or `--gtest_filter`) over the full `ctest` suite while iterating; run the full suite before considering a change done.
