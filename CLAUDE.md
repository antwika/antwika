# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

A C++23 game/engine project built with CMake + Conan and tested with GoogleTest/CTest, developed inside VS Code Dev Containers (GNU, LLVM, MinGW) for a fully reproducible toolchain.
Read [`README.md`](README.md) for the full picture, [`REQUIREMENTS.md`](REQUIREMENTS.md) for the MoSCoW-phrased requirements behind the design, and **[`docs/STYLE_GUIDE.md`](docs/STYLE_GUIDE.md) for the full coding style** (naming, formatting, includes, error handling, testing conventions, CMake conventions) — it is authoritative and detailed; don't restate it here, follow it.
`docs/` holds only the rules that are not about one library -- the style guide, schema versioning, resizable windows, hover, and confirming an unreachable branch -- and everything a listing of it shows is still normative; a plan document is deleted once the work it describes has shipped.
Everything that *is* about one library or one application lives in [`wiki/`](wiki/Home.md), one page each, which is also the project's public face; the reasoning worth keeping otherwise moves into the code, into this file, or into a `blog/` post.
**This file is deliberately short**: it says what an agent needs in context to work anywhere in the tree, and every module's design and rationale lives on its wiki page instead.

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

Or in VS Code: `Ctrl+Shift+B` runs [`scripts/build.sh`](scripts/build.sh), which is that sequence plus whichever backends are selected (see [`.vscode/tasks.json`](.vscode/tasks.json)).

```sh
scripts/select_backend.sh gfx sdl3     # Tasks: Run Task > Select gfx backend
scripts/select_backend.sh sound sdl3   # Tasks: Run Task > Select sound backend
scripts/build.sh                       # Ctrl+Shift+B
```

A selection is a line in the untracked `.vscode/<subsystem>-backend`, read on every build, so it is made once rather than on every build and is per-developer rather than the repository's.
Input is not selectable there, since its option is `auto` and follows graphics.
**That path keeps one build folder**, `build/` under the `conan-release` preset, whatever is selected -- which is why it passes no `build_folder_vars` conf and needs no separate install of the default configuration.
The by-hand commands below and CI both keep a folder per backend instead, because CI's legs run in parallel and cache separately; a developer switches one at a time, so the cost of one folder is a reconfigure and a mostly-full rebuild on a switch, and the benefit is not owning a `build-*` tree per permutation.

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
A directory selected for one subsystem only builds that subsystem's target, which is why each `backends/<name>/CMakeLists.txt` guards its targets separately.
A selection naming a directory that implements no such subsystem is refused at configure time rather than failing much later at link.

Sound has its own selection too, `-o sound_backend=` and `ANTWIKA_SOUND_BACKEND`, with values `null` and `sdl3` -- and it defaults to `null` rather than to `auto`.
Input follows graphics because a window nobody can click is useless; sound is orthogonal, and `auto` would mean every existing `-o gfx_backend=sdl3` build silently began opening an audio device.
`raylib` is absent from the option's values because it does not implement that seam, which is the cheapest possible enforcement: Conan refuses an unlisted value before anything is downloaded.
A lockfile is per *framework* rather than per subsystem, so `-o sound_backend=sdl3` uses `conan-sdl3.lock` like any other sdl3 build.

```sh
conan install . -of build-sdl3 -o gfx_backend=sdl3 -o sound_backend=sdl3 \
  -c tools.cmake.cmake_layout:build_folder_vars="['options.gfx_backend']" \
  -pr:b=./profiles/build/${CONAN_PROFILE} \
  -pr:h=./profiles/host/${CONAN_PROFILE} \
  --build=missing -s build_type=Release --lockfile=conan-sdl3.lock
```

**Updating the lockfiles** after a dependency bump in `conanfile.py` (what Renovate leaves stale):

```sh
scripts/update_lockfiles.sh   # or: Tasks: Run Task > Update Conan lockfiles
```

It re-resolves every lockfile from scratch against every profile under `profiles/host/`, since CI builds all of them against the same files.

Set `SDL_VIDEODRIVER=dummy` to run the SDL build with no display, or use `xvfb-run` for any backend, which is how the conformance suite is exercised without a desktop session.
`SDL_AUDIO_DRIVER=dummy` is the sound equivalent, and the sound suite is run **without** `xvfb-run` on purpose: a sound backend that needed a display would be one that had quietly taken a dependency on video, and running it under Xvfb is exactly what would hide that.
`raylib` reports `maxWindows() == 1`, since it keeps its one window in global state; the conformance suite skips its multi-window tests for such a backend rather than failing them.

**Run a single test binary / test case:**

```sh
ctest --test-dir build -R antwika_replay_tests --output-on-failure
build/bin/antwika_replay/antwika_replay_tests \
    --gtest_filter='ReplayReaderTest.*'
```

**Run the apps:**

```sh
build/bin/antwika_game/antwika_game                      # empty grid, runs until quit
build/bin/antwika_game/antwika_game --record demo.replay # or --replay demo.replay
build/bin/antwika_life/antwika_life                      # runs until stopped
build/bin/antwika_life/antwika_life --record demo.replay
build/bin/antwika_task_worker/antwika_task_worker --record demo.replay
build/bin/antwika_poker/antwika_poker --record demo.replay
build/bin/antwika_sudoku/antwika_sudoku [--puzzle my-puzzle.txt]
build/bin/antwika_gfx_demo/antwika_gfx_demo              # runs until the window is closed
build/bin/antwika_gfx3d_demo/antwika_gfx3d_demo          # spinning cube, 900 frames
build/bin/antwika_sound_demo/antwika_sound_demo          # eight notes; silent under null
build/bin/antwika_sound_demo/antwika_sound_demo --file my.wav  # or play a file instead
build/bin/antwika_tower_defence/antwika_tower_defence    # or --record / --replay
build/bin/antwika_ui_demo/antwika_ui_demo                # every antwika::ui element, 1500 ticks
build/bin/antwika_companion/antwika_companion            # tap to feed it
build/bin/antwika_atlas_editor/antwika_atlas_editor      # blank 1024x256 sheet
build/bin/antwika_atlas_editor/antwika_atlas_editor \
    --image src/apps/game/assets/atlas.png --out mine.png
```

**Every application gets a directory of its own under `bin/`**, holding the executable, whatever it opens and -- on MinGW -- the runtime DLLs it needs to start, all put there by `antwika_bundle_app()` in [`cmake/AntwikaModule.cmake`](cmake/AntwikaModule.cmake).
A test binary goes to the directory of the module that owns it, put there by `antwika_bundle_test()` in the same file, which also registers the cases with CTest.
An application finds what it opens through `antwika::app::assetPath()` rather than through the working directory or a path baked in at configure time; [`wiki/libraries/app.md`](wiki/libraries/app.md) says why both of those failed.

Several applications have no end of their own and run until the window is closed or a replay dispatches `engine.stop`, and the `null` backend reports neither.
So `Ctrl+C` is what ends one on a default build -- and since a `--record` run only writes its file once the run has ended, a run killed that way saves nothing.
Each application's flags, pacing and stopping conditions are on its own page under [`wiki/apps/`](wiki/Home.md).

**Coverage build** (separate `build-coverage/` dir, GNU/LLVM only — not MinGW):

```sh
cmake --preset conan-coverage
cmake --build build-coverage -j24
ctest --test-dir build-coverage -j"$(nproc)"
gcovr --root . --filter 'src/.*' --exclude '.*/tests/.*' --print-summary build-coverage
```

CI requires 100% line/function/branch coverage on the GNU leg (`scripts/check_full_coverage.py`); see [`docs/confirming-unreachable-branches.md`](docs/confirming-unreachable-branches.md) before excluding any line with `GCOVR_EXCL_LINE`.
Instrumented tests contend on the `.gcda` writes, so `-j` scales sublinearly, but the totals it reports are identical to a serial run's.

**Three things keep that step from getting slow again, and each is load-bearing.**
The coverage build is `-O0`, which is 20x slower than the release build at everything, so a wide soak belongs in an optimised build as `LevelGeneratorTest`'s does -- and `-Og` is not the escape, because inlining merges functions and rewrites branches and takes the gate from 100% to 92.4%.
A test looking for where a widget ended up asks `ui::Frame::rects`, never a sweep over the canvas re-running `describe()` per pixel -- `src/apps/game/tests/WidgetPixel.hpp` is the one helper for it, and the sweep it replaced was 163,840 layouts per lookup and 48 seconds of one fixture.
Test targets share one precompiled header, built once and reused (`REUSE_FROM` at the bottom of the root `CMakeLists.txt`), because a header per target costs about what it saves; it is scoped off the application sources a test target compiles, since force-including `<gmock/gmock.h>` into those changes the code the gate is measuring.

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
**Every library and every app has one page under [`wiki/`](wiki/Home.md)**, and that page is where its design and the reasoning behind it live; the lists below are pointers into the wiki rather than a second copy of it.
Read the page for a module before changing it, and put any new reasoning worth keeping back onto that page rather than here.

### The rules that cross every module

None of these is local to one library, which is why they are stated here and not only on a wiki page.
Breaking one is the class of mistake that looks fine live and surfaces as a divergent replay a long way from its cause.

- **A replay reproduces a run by construction, not by convention**: `simulation::EngineLoop` is the one code path shared by live and replay runs, and the two differ only in what implements `ITickEventSource` (see [`blog/001-building-a-deterministic-replay-system.md`](blog/001-building-a-deterministic-replay-system.md)).
- **Only externally-supplied input is persisted**, and anything a sink or a system derives from it is regenerated -- a click is recorded and the tile it lays is not, so no app may define an event for something it can work out again.
- **What lands in a recording is decided by where the recorder sits** rather than by a list of names it skips: `event::TickEventRecorder` records unconditionally, so any thinning of the stream must sit upstream of it and never in a backend.
- **Anything the meaning of a click depends on is simulation state**, which is why `apps/game`'s camera and `apps/atlas_editor`'s `CanvasView` are, and why a zoom is an index into a table of whole sizes rather than a scale factor (see [`blog/013-the-camera-is-simulation-state.md`](blog/013-the-camera-is-simulation-state.md)).
- **A layout or a hit-test uses `IWindow::configuredSize()`**, never the size a window reports, or a window resize would change what a recorded click means -- see [`docs/resizable-windows.md`](docs/resizable-windows.md).
- **Rendering is a write-only projection**: no pixel, texture, mesh or measurement is ever read back into the tick loop, and no floating-point value from the render side may enter anything a replay reproduces (see [`blog/012-a-window-that-cant-talk-back.md`](blog/012-a-window-that-cant-talk-back.md)).
- **A pointer position read off `input::PointerHintChannel` may decide what is drawn and nothing else** -- it is deliberately in no recording, so a live run and its replay do not agree on it, and folding one into simulation state diverges silently; see [`docs/hover-is-not-simulation.md`](docs/hover-is-not-simulation.md).
- **A UI is described and resolved inside the tick path**, downstream of the recorder and never in a renderer, so a replay stores the click and regenerates which widget it activated; no `ui.*` event name may ever exist.
- **Every persisted schema states its version** and is read `parse -> read version -> migrate -> validate -> decode`, with an injected `replay::MigrationChain` of single-step migrations; see [`docs/schema-versioning.md`](docs/schema-versioning.md).
- **Determinism is total ordering**: every tie in a search, a sort or a draw breaks down to something total (ascending `NodeId`, `x + y` then `x`), and randomness only ever comes from an injected `antwika::rng` seeded from something already persisted.
- **One exception type per failure category**, declared by the module that owns the failure.

### Libraries

- [`engine`](wiki/libraries/engine.md) steps one fixed tick at a time and is domain-agnostic, knowing nothing about any app's state.
- [`event`](wiki/libraries/event.md) is the one, uniform event mechanism, with app-defined dot-namespaced kinds (`game.score_increment`, `life.toggle_cell`, `task.submit`) alongside the built-in `engine.tick` and no special-casing between them.
- [`simulation`](wiki/libraries/simulation.md) is the loop itself -- `EngineLoop`, the `ITickEventSource` seam it reads through, `TickPacer` and `WindowInputSource`.
- [`replay`](wiki/libraries/replay.md) is the JSON format, its migrations, the `--record`/`--replay` flags and `ReplaySource`; it depends on `simulation` and never the reverse, so a live run links no JSON, no schema validator and no migration chain.
- [`ecs`](wiki/libraries/ecs.md) is the double-buffered `World`/`SystemScheduler`, where a system stages writes that only land at `commit()` -- see [`blog/003-an-entity-component-system-with-nowhere-to-hide-a-mutation.md`](blog/003-an-entity-component-system-with-nowhere-to-hide-a-mutation.md).
- [`ecs_commons`](wiki/libraries/ecs_commons.md) is the vocabulary half of that (`GridPosition`, `Velocity`, `Lifetime`, `Name`, `Tag<Kind>` and the systems acting on them), kept separate so an app wanting the scheduler does not link a countdown it never uses.
- [`scheduler`](wiki/libraries/scheduler.md) is a deterministic, priority-ordered, budget-bounded job queue in which dependency cycles are unreachable by construction -- see [`blog/006-a-job-scheduler-and-a-worker-pool-that-cant-lie-to-itself.md`](blog/006-a-job-scheduler-and-a-worker-pool-that-cant-lie-to-itself.md).
- [`holdem`](wiki/libraries/holdem.md) is the poker engine, whose `evaluate()` scores 5-7 cards into one comparable `HandValue` over four per-suit 13-bit rank masks -- see [`blog/010-a-poker-hand-in-one-number.md`](blog/010-a-poker-hand-in-one-number.md).
- [`wfc`](wiki/libraries/wfc.md) is a dependency-free, deterministic Wave Function Collapse solver over a flat index-addressed array, with all geometry expressed as `IConstraint`s and no grid concept inside -- see [`blog/005-wave-function-collapse-that-never-guesses.md`](blog/005-wave-function-collapse-that-never-guesses.md).
- [`pathfinding`](wiki/libraries/pathfinding.md) is an A* that knows nothing about grids, and its open set orders down to ascending `NodeId` so an equal-cost route resolves the same way on every run and every toolchain.
- [`rng`](wiki/libraries/rng.md) is the one source of pseudo-random bits, and `IRng::next()`'s 64 raw bits are the whole interface -- there is deliberately no distribution, since `<random>`'s are not portable across standard libraries.
- [`animation`](wiki/libraries/animation.md) resolves which frame to show as a pure function of a tick the caller already has; there is deliberately no `Animator` you advance, since that would be simulation state hidden in a renderer.
- [`time`](wiki/libraries/time.md) is the fixed-tick `Tick` type and `IClock`/`SystemClock`.
- [`gfx`](wiki/libraries/gfx.md) abstracts windows, 2D drawing, textures and -- through the sibling `IRenderer3D` rather than more methods on `IRenderer` -- meshes, so no code under `src/` names a graphics framework.
- [`ui`](wiki/libraries/ui.md) is an immediate-mode UI over `gfx` that retains nothing between frames: activation is on the press, and focus, text and dropdown state are all passed through by the caller.
- [`input`](wiki/libraries/input.md) abstracts a keyboard and a pointer as symbolic *edges* rather than held state, and deliberately does not depend on `gfx` in source, so a session recorded under one backend replays under another.
- [`font`](wiki/libraries/font.md) turns TrueType bytes into metrics and coverage masks, and depends on no other module of this project.
- [`i18n`](wiki/libraries/i18n.md) is a compiled-in catalogue keyed by a symbolic `MessageId` rather than by the English string, with a total lookup that never throws; a `Translator` is injected like any other collaborator, and an application that hit-tests a layout fixes its locale in `main()` so the language cannot become simulation state.
- [`sound`](wiki/libraries/sound.md) decodes, mixes and plays PCM behind a backend seam, and owns no thread, lock or queue: a device renders only when `pump()` asks it to, on the thread that asked.
- [`app`](wiki/libraries/app.md) is what every `main()` shares -- `runRecorded()`, `assetPath()`, `FramePacedSource`/`IFramePass` and `pointerFrom()`/`hoverFrom()`.
- [`cli`](wiki/libraries/cli.md) is the flag parsing, depending on nothing at all; one `FlagSpec` table is both the parser's input and the help text's, and a program parses once against one concatenated table.
- [`log`](wiki/libraries/log.md) is composable logging with no global state.

Backends live under [`backends/`](backends/), are selected at build time, and are the only place a concrete framework is named; `null` implementations that belong to the coverage gate live in the library instead.

### Applications

Each app owns its state and how events mutate it -- the engine has no opinion here.

- [`game`](wiki/apps/game.md) is an isometric city built with the mouse and the most complete composition in the tree: a reducer, an ECS, an economy, A* inside the tick, a texture atlas ([`game-texture-atlas.md`](wiki/apps/game-texture-atlas.md)), a UI toolbar, versioned saves and frames drawn between ticks.
- [`life`](wiki/apps/life.md) is Conway's Game of Life in an `ecs::World`, where dragging over cells toggles them and holding the button pauses the generations.
- [`task_worker`](wiki/apps/task_worker.md) is a worker pool pulling from `scheduler` with each tick's idle-worker count as its budget.
- [`poker`](wiki/apps/poker.md) is a no-limit hold'em cash game on `holdem`, drawing its table from one layout that the art is placed *from* -- see [`blog/011-writing-a-hand-history-the-rest-of-the-world-can-read.md`](blog/011-writing-a-hand-history-the-rest-of-the-world-can-read.md).
- [`tower_defence`](wiki/apps/tower_defence.md) generates its level with `wfc` and walks mobs along it, arranging the tile alphabet so a linear path is a property of the alphabet rather than something checked for afterwards.
- [`companion`](wiki/apps/companion.md) is a tamagotchi whose whole design is that *when* a tap lands is what decides what it means, with a versioned save carried between live sessions.
- [`atlas_editor`](wiki/apps/atlas_editor.md) is a pixel editor for the sheet `game` blits, and is an ordinary application of the tick loop with no undo -- replaying a session up to a point is the undo this design has.
- [`sudoku`](wiki/apps/sudoku.md) is unrelated to the tick loop and is `wfc`'s showcase, expressing the 81-cell puzzle as `AllDifferentConstraint`s over a flat array.
- [`gfx_demo`](wiki/apps/gfx_demo.md), [`gfx3d_demo`](wiki/apps/gfx3d_demo.md), [`sound_demo`](wiki/apps/sound_demo.md) and [`ui_demo`](wiki/apps/ui_demo.md) are the showcases for `gfx`, its 3D half, `sound` and `ui`.

## Notes for AI agents

- **Always work in a separate git worktree, never directly in the primary checkout.** Before making any change, create/enter a dedicated worktree for the task (`git worktree add .worktrees/<task> -b <task>`), do all editing, building, and testing there, and only merge back when the work is done.
  This keeps `main` clean and lets several tasks build in parallel without clobbering each other's `build/` directory.
  `.worktrees/` is the agreed home for them and `.gitignore` covers it, so a worktree and its build output never show up as untracked state in the primary checkout; `.claude/` and `core.*` are ignored for the same reason.
- Read the wiki page for a module before changing it, and the blog posts under `blog/` before changing a library's core abstraction — they are design write-ups for *why* a piece was built the way it was, written after the fact, and usually explain a constraint that isn't obvious from the code alone.
- Prefer running a single test binary (or `--gtest_filter`) over the full `ctest` suite while iterating; run the full suite before considering a change done.
