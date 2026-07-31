# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

A C++23 game/engine project built with CMake + Conan and tested with GoogleTest/CTest, developed inside VS Code Dev Containers (GNU, LLVM, MinGW) for a fully reproducible toolchain.
Read [`README.md`](README.md) for the full picture, [`REQUIREMENTS.md`](REQUIREMENTS.md) for the MoSCoW-phrased requirements behind the design, and **[`docs/STYLE_GUIDE.md`](docs/STYLE_GUIDE.md) for the full coding style** (naming, formatting, includes, error handling, testing conventions, CMake conventions) — it is authoritative and detailed; don't restate it here, follow it.
`docs/` holds only documents that are still normative, so everything a listing of it shows is current; a plan document is deleted once the work it describes has shipped, and the reasoning worth keeping moves into the code, into this file, or into a `blog/` post.

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
build/bin/antwika_replay_tests --gtest_filter='ReplayReaderTest.*'
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
build/bin/antwika_sound_demo/antwika_sound_demo my.wav   # or play a file instead
build/bin/antwika_tower_defence/antwika_tower_defence    # or --record / --replay
```

**Every application gets a directory of its own under `bin/`**, holding the executable, whatever it opens and -- on MinGW -- the runtime DLLs it needs to start, all put there by `antwika_bundle_app()` in [`cmake/AntwikaModule.cmake`](cmake/AntwikaModule.cmake).
Two applications ship an `atlas.png` and three a `demo.json`, so one shared `bin/` was one atlas and one demo replay between them the moment either had to sit beside its binary.
An application finds those files through `antwika::app::assetPath()`, which asks the operating system where the running executable is (`/proc/self/exe`, `GetModuleFileNameW`) rather than reading the working directory -- so starting one from anywhere still works, and `antwika::app` is the one place under `src/` that names an operating system.
What this replaces is a path baked in at configure time, which was the *building* machine's path: right on the machine that built it, and a directory that does not exist on any other, so every cross-built executable that opened anything died on its first line.
Test binaries stay directly in `bin/`, since they open nothing.

`antwika_tower_defence` opens a window, draws the level each tick and takes mouse input.
Like `antwika_life` it has no end of its own: it runs until the window is closed, or until a replay dispatches `engine.stop`.
A headless build reports neither, so `Ctrl+C` is what ends one -- and a `--record` run only writes its file once the run ends.

`antwika_life` opens a window, draws the board each tick, and takes mouse input.
It has no end of its own: it runs until the window is closed, or until a replay dispatches `engine.stop`.
A headless build reports neither, so `Ctrl+C` is what ends one -- and a `--record` run only writes its file once the run ends.
Both the windowed and the headless run are paced through `TickPacer`, since a run that never ends would otherwise go flat out.

`antwika_poker` also opens a window and draws the table each tick.
`--tick-delay-ms <n>` holds each frame for `n` ms; it defaults to 1000, since one tick is one poker action and a session is unwatchable without it, and `--tick-delay-ms 0` runs flat out.
Naming the flag with a positive value *also* keeps the final frame up until the window is closed, which is a separate answer from the pacing (`WindowSetup::holdFinalFrame`) precisely so the default paced run still ends under the `null` backend, which never reports a close and would otherwise wedge.
A real backend needs a display, so use `SDL_VIDEODRIVER=dummy` or `xvfb-run` without one.

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
The coverage build is `-O0`, which is 20x slower than the release build at everything, so a test that repeats expensive work pays 20x for it -- `-Og` is not the escape, because inlining merges functions and rewrites branches and takes the gate from 100% to 92.4%.
So a wide soak belongs in an optimised build, as `LevelGeneratorTest`'s does.
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
  `SchemaVersionError` narrows the first to the one cause a caller may want to word differently: a document this build cannot bring to the current schema version.
- **Every persisted schema states its version**, named in `antwika/replay/SchemaVersion.hpp` rather than written at a call site — the replay document in its own `"version"` member, the tick-event schema in its `$id`, since an event repeats thousands of times per document and its revision is fixed by the document holding it.
  Reading is `parse -> read version -> migrate -> validate -> decode`, and validating *after* migrating is what lets exactly one schema exist rather than one per revision.
  A document with no version member is version 1, which is what every file written before this says.
  `MigrationChain` applies single-step `IMigration`s (N to N+1) until a document reaches current — single-step so the number of migrations stays linear in bumps rather than quadratic — and it is generic over an `nlohmann::json` and a version key, so a save file uses the same mechanism with its own list and its own current version.
  Chains are constructed and injected, never registered globally; `standardReplayMigrations()` is the replay document's factory, and `ReplayReader` takes one.
  See [`docs/schema-versioning.md`](docs/schema-versioning.md) for what counts as a breaking change and how to bump.
- `antwika::replay::ReplayCli` parses the `--record <path>` / `--replay <path>` flags shared by every app's `main.cpp`.

**Application state**: each app owns its state and how events mutate it — the engine has no opinion here.

- `apps/game` is an isometric grid you build on with the mouse: left-click lays a path tile, right-click drops a walker onto one, middle-drag pans and the wheel zooms.
  Walkers advance one cell every `game::kTicksPerStep` ticks along the paths, counted down in each walker's own component rather than off the tick number, preferring a right turn at an intersection and reversing at a dead end -- both of which fall out of one preference order in `game::nextFacing()` rather than two rules.
  **A walker slides between two cells rather than jumping, and the frames that show it happening are drawn outside the tick.**
  `antwika::app::FramePacedSource` is an `IReplaySource` decorator that draws `framesPerTick - 1` frames in the gap before each tick's events are read, then hands back what the source it wraps returned, unchanged -- so it is a pure observer of the stream in exactly `input::PointerHintSource`'s sense, and this app's own `TickPacer` is gone because one frame a tick is the same thing it did.
  What a frame is handed is an `app::IFramePass`, whose only method takes an `animation::Progress` and **no `World`, no `Tick` and no dispatcher**: a pass between two ticks cannot change what the simulation computes because it is given nothing it could change, which is structural rather than a promise.
  `RenderSystem` implements both interfaces, snapshotting in `update()` and redrawing that snapshot in `draw()`, and that cached `SceneSnapshot` is the app's only render-side mutable state -- safe for the one reason above, so handing `draw()` a `World` would quietly remove the guarantee.
  Where a walker came from is `game::Walker::from`, and it is **simulation state rather than a render channel**: unlike a pointer hint, a live run and its replay have to agree on it, since both draw the same picture from it.
  Reconstructing it as `step(at, opposite(facing))` is right mid-run and wrong exactly where there was no previous cell -- freshly placed, freshly spawned, restored from a save -- which is why it is a `std::optional<Cell>` rather than a cell that lies.
  The picture and the state part company at `SceneSnapshot`: `WalkerSprite` carries `from` and `ticksIntoStep` for drawing, while `WalkerView` stays what `GameSummary` and `SaveGame` hold, since a render-only field would otherwise land in a persisted schema and in the value `ReplayDeterminismTest` compares -- the same rule that keeps a road's link mask out of the snapshot.
  The interpolation itself is `game::WalkerMotion.hpp`, exact rational arithmetic through `animation::interpolate`, so the same frame of the same tick is the same pixel on every toolchain; `FrameRateDeterminismTest` is what pins that drawing more often cannot change what a run computes.
  **The buildings are an economy rather than a timer.**
  `game::BuildTool` is the palette and `game::BuildingKind` is the model, with `buildingKindOf()` the one crossing between them -- they used to be one enumeration, which gave every per-building table a `Road` entry that could only ever be wrong.
  A house consumes what is delivered to it; the four sources each send one `game::WalkerKind` out; and both facts are arithmetic over the shared declaration order rather than a switch, so a sixth kind is two enumerators and a tile.
  `game::BuildingSystem` runs deliveries, drain, risk and demolition, and the one subtlety worth stating is that **two walkers reaching one building in a tick add up rather than racing**: each would otherwise read the same committed amount and stage a write, so the last would win and quietly halve a delivery, which is why every change accumulates in a map and each building is written once.
  Every period derives from one `game::kTicksPerSecond` rather than being a constant per rule, and each countdown lives in the building's own component so two buildings put up a tick apart never fall into lockstep.
  **A building may have one walker out at a time**, and it holds the handle rather than counting walkers -- a lookup instead of a scan of every walker per building per tick.
  That handle is a cache and `world.alive()` is the authority, which is safe because `ecs::EntityManager` never reuses an index, so a stale handle can only ever be *dead* rather than somebody else.
  A building with no road beside it holds its countdown at zero rather than resetting it, so laying a road beside a long-neglected source releases one walker and not a queue of them, and `game::kWalkerLimit` stays as a backstop.
  **Once a walker's roaming budget is spent it either walks home or it is gone**, and that single rule is what bounds the population.
  Every awkward case collapses into its last arm -- a walker nobody sent, one whose building has burned down, one walled off from home, one whose road was demolished under it -- so all four are answered by destroying the walker rather than by four rules, and none of them is an error.
  A right-click walker therefore expires too; nothing in the app is immortal.
  The route home is `game::stepTowards()`, an A* over the roads through `antwika::pathfinding`, re-searched each step with only its first move used: a route cannot live in a component, and one held in a system would be state outside the `World` that a save does not cover.
  It is replay-safe because that search orders down to ascending `NodeId`, which is exactly why the extent is passed in rather than derived from the roads -- a bounding box computed from whichever roads happen to exist would renumber every node as one was laid, and with it the tie-break.
  `game::BuildingIndex` is `PathIndex`'s counterpart and exists for the same reason, with two writers and only two: `GridSink` records a block as it builds on one and `BuildingSystem` clears one as it demolishes.
  **A building covers a square block of cells rather than one**, sized by `game::footprintOf()` -- a table keyed by kind rather than a field on the component, because a field could disagree with the kind that placed it and because the ghost has to know the size *before* any entity exists.
  **Square is load-bearing rather than a simplification**: a cell's box is `2 * halfWidth` by `2 * halfHeight` and `halfHeight` is `halfWidth / 2`, so every footprint's box comes out 2:1 -- the same shape as one atlas tile.
  A square block therefore *is* a diamond and its art is one ordinary tile scaled up with no geometric error and no atlas work at all, where a 2x3 block is a hexagon that would need a source rect of its own, a half-tile-quantised band in the sheet, and a much heavier contract with whoever draws it.
  `game::canPlace()` is the one statement of what a block will land on, used by `GridSink` *and* by `ghostFor()`, so what a preview promises and what a click delivers cannot drift; a refused block is shown reddened rather than hidden, since a refusal somebody can see is one they can act on.
  Two consequences fall out of blocks that one-cell buildings never had.
  **Painter's order stopped being optional**: two one-cell buildings could never overlap, so placement order was as good as any, and a block drawn before what is behind it is simply the wrong picture -- so `snapshotOf()` sorts on `x + y` with a tie-break on `x`, which is screen depth and is total.
  And a walker reaches a building by *any* cell of its block, which is why `spawnCellFor()` walks the whole perimeter and `stepTowards()` makes every cell of the goal passable.
  There is deliberately no guard against one walker serving one building twice: two of a cell's four neighbours being in one rectangle would put the cell in it too, so it would be a road under a building, which nothing places.
  The plain `GameState` struct and its `GameStateReducer` are still there, folding `game.score_increment` alongside the grid.
  **The camera is simulation state, not render state**, which is the load-bearing decision: a click arrives as a pixel, and which cell it means depends entirely on the camera, so a renderer-owned camera would leave a replay resolving recorded clicks against a different view.
  That is also why zoom is an index into a table of whole tile sizes rather than a scale factor, why `game::floorDiv()` exists instead of `operator/`, and why the projection is anchored to the camera's pan rather than the canvas centre -- anchoring to the centre would make a window resize change which cell a pixel means.
  **The app defines no event for placing anything**: a click is the input, `game::GridSink` turns it into a placement inside the tick path, and the replay stores the click and regenerates the placement -- persisting both would lay two tiles per click.
  Every tile is one blit from one texture atlas (`src/apps/game/assets/atlas.png`), addressed through `game::TileAtlas.hpp`: `game::GridScene` draws no shape of its own, so the lattice is painted into the ground tile's own edges rather than being lines the scene places, and a junction is one of sixteen road tiles indexed by which neighbours the cell joins.
  That mask is worked out in the scene from the snapshot's paths, which arrive in ascending order, so a neighbour is a binary search rather than a second index to keep in step -- and it stays out of `SceneSnapshot` and `GameSummary`, since which tile a road shows is a picture, not state a replay has to reproduce.
  **The picture is hand-drawn art and is the source of truth for how the game looks**; it used to be generated by a script CI re-ran to prove the committed PNG had not drifted, and that script is gone, so nothing rebuilds the atlas and editing it is editing the art.
  `TileAtlas.hpp` remains the address map, and it says where a tile lives arithmetically rather than as a table of rectangles, so there is no list in the header that could disagree with the picture -- repainting a tile is therefore free, and *moving* one is not.
  What is left to catch a mistake is the `static_assert`s in that header, `TileAtlasTest`, and a check at startup that the PNG really is `kAtlasSize`; see [`docs/game-texture-atlas.md`](docs/game-texture-atlas.md) for what an artist has to produce.
  A toolbar of zoom, reset-view and pause buttons is drawn over the grid by `game::Toolbar`, described and resolved once per tick by `game::UiSink` -- registered *before* `GridSink`, so a press is resolved against the bar before the grid sees it -- and painted last by `RenderSystem`.
  It defines no event either, for the same reason: what a recording holds is the click.
  **The pause is `apps/life`'s answer to the same question**: `game::PauseGatedSystem` wraps a system and stages nothing while `game::PauseState` says the run is held, exactly as `life::DragPausedSystem` does, and only those systems stop.
  Which ones is the product decision, and it is three: `WalkerSystem`, `BuildingSystem` and `SpawnSystem`, the three that make a city move on its own.
  The tick, the commit, every observer, the toolbar, the camera and placement all carry on, so **a paused city can still be panned over, zoomed into and built on** -- this is a build pause rather than a freeze, and a pause nobody could act on would just look like a hang.
  It composes with `ModeGatedSystem` rather than replacing it, since a run is paused *and* in a mode and either gate alone answers only its own question.
  `PauseState` is simulation state in exactly the sense the camera and the selected tool are, toggled by `UiSink` inside the tick path, so a replay pauses on precisely the ticks the live run paused on and nothing about a pause is ever persisted.
  **The bar reports the tick and a corner of the screen reports the frame rate, and the two are in different places for one reason.**
  The tick is simulation state, so it is a label on the bar, described in the tick path off the `TickEvent` being handled and regenerated by a replay like everything else there.
  Frames per second is measured against a wall clock, which says how fast the machine is -- so `game::FrameMeter` is render-side only, is handed an injected `time::IClock` rather than reading one, is counted in `RenderSystem::draw()` (the one thing that runs exactly once per frame, `app::FramePacedSource`'s between-tick frames included), and is drawn by `game::describeFps()` straight from the renderer.
  It reaches no sink, no system, no `SceneSnapshot`, no `GameSummary` and no save, which is the whole of its safety condition -- the same one `input::PointerHintChannel` is held to, arrived at from the other side.
  `RenderSetup::fps` is optional and absent by default, so a run with no wall clock to offer draws no readout and every test whose subject is the picture is spared one.
  `game::UiOverlay` is the one fact the three share, and it owns the canvas the bar is laid out against (the size the window was *asked* for) so nothing can lay it out against one size and hit-test it against another.
  What the bar covers, it covers from the grid too: `GridSink` skips a press or a scroll the overlay reports as covered, though not a movement, so a pan begun on the grid carries on across the bar.
  **A button here lights up on the press rather than on approach**, and that is `input::IdleMotionSource` in this app's chain rather than anything `antwika::ui` decides: idle pointer movement is held back until something reads it, so a hover appearance updates only when a button, a wheel or a key arrives.
  Clicking is unaffected, since the gate releases the latched movement ahead of the press and a press carries its own position.
  **The placement ghost is the exception, and it is not a trade any more**: `game::ghostFor()` works it out on the render side from `input::PointerHintChannel`, which carries a free-moving pointer without putting one byte in a recording, so the app keeps the gate *and* draws a hover -- what the button still does not do is light up on approach, because that is `antwika::ui` resolving against the event stream.
  The ghost is therefore a value the renderer computes each frame and hands to `SceneSnapshot`, never a component staged into the `World`: a replay does not reproduce the channel, so folding a hint into simulation state would make a run and its replay disagree there silently.
  No sink may read it, and "the ghost is over the toolbar" is worked out *from* `UiOverlay` rather than the other way round, since `UiOverlay` is derived from recorded input and the hint is not.
  It starts on an empty grid and loads nothing unless `--replay` says so, so what a session contains is what somebody clicked.
  It runs until Escape is pressed or the window is closed -- both of which are input, so both are recorded and both replay.
  Neither reaches the `null` backend, so that build runs until interrupted, and a `--record` there never gets to save.
  `src/apps/game/replays/demo.json` is a sample session to pass to `--replay`.
  **A save is version 2 and carries the buildings**, which version 1 did not: their kinds, stock, risk and all three countdowns, since countdowns reset on load are the lockstep they exist to avoid.
  The building/walker link is persisted **as a pair of array indices rather than as an `ecs::Entity`**, because `EntityManager` hands ids out from a monotonic counter and a restore destroys and recreates everything, so a raw handle would name nothing on the way back in.
  Reading refuses an index past the end of the array it points into, and refuses a pair that disagree about each other, rather than repairing either -- a repaired save is a session somebody never had.
  Restoring creates every entity *before* adding any component, because `create()` is immediate where `add()` is staged, so a link has to be built into the component rather than written onto it afterwards.
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
  The art it is drawn from is one texture atlas (`src/apps/poker/assets/atlas.png`), addressed through `poker::PokerAtlas.hpp` and generated by [`scripts/generate_poker_atlas.py`](scripts/generate_poker_atlas.py) from the same slot numbers the header names, exactly as `apps/game`'s is; CI fails if the committed PNG has drifted from the generator.
  A card is a frame plus a rank glyph plus a suit glyph rather than one of fifty-two faces, and both glyphs are drawn white so the tint decides whether a suit is red or black -- which is what keeps the atlas at seventeen glyphs.
  `TableScene` returns the art as a second value beside its `ui::Frame` -- a `std::vector<poker::ArtBlit>` from `describeArt()`, painted *before* `ui::paint()` so the names and stacks read on top of it -- because `ui::DrawList` holds rectangles and text and no texture command, and inventing one there would make every `antwika::ui` caller pay for it.
  **There is one layout, and the art is placed from it**: `describe()` names a container for every card, seat and badge (`poker::widgets` in `TableWidgets.hpp`), and `describeArt()` is handed that frame's `ui::WidgetRects` and blits into the rectangles it reports -- an id the frame did not declare draws nothing, since inventing a rectangle for it is the second layout this deletes.
  Working the art's own geometry out beside the UI's is exactly what once put the seat plates a row below the boxes they plate and printed a card's rank across the gap between two other cards, so `TableAlignmentTest` asserts that every card's text falls inside the card it belongs to and inside no other, over five canvas sizes and four seat counts.
  How big a card is is therefore stated once, in the layout, in the font's own metrics, and its face is a slot of the atlas rather than a fill -- a colour laid over the blit would be the same picture painted twice.
  `main.cpp` decodes the PNG and `PokerRoom` uploads it, since a texture belongs to the renderer that made it and the room is what owns the window; a null atlas is an ordinary state, which is what lets a test assert a session's chip counts without any art at all.
  The only route back in is `poker::WindowCloseSource`, an `IReplaySource` decorator that appends `engine.stop` once the window has gone, so a close is ordinary replay input and lands in a `--record` file like anything else.
- `apps/tower_defence` generates its level with `antwika::wfc` and walks mobs along it.
  The interesting part is that WFC is a constraint solver, not a path guarantee, and a linear non-intersecting path is a global property plain adjacency rules will happily break.
  It is arranged in three layers rather than checked for afterwards.
  The tile alphabet in `LevelTile.hpp` has no symbol open on more than two sides, so a T-junction or a crossroads is not expressible.
  Exactly one `Start` and one `End` are allowed anywhere in the wave and both are pinned to a border cell, so the solution is a union of simple cycles plus exactly one simple path whose two ends are the only degree-one cells -- walking out of `Start` therefore always arrives at `End`.
  Any cell the walk misses is a stray cycle, which `generateLevel()` erases rather than rejects, so generation never reseeds for the sake of linearity.
  `LevelGeneratorTest` asserts that property over forty seeds -- but only in an optimised build, because generating one level costs ~2.9s under the coverage build's `-O0` against ~0.14s at `-O2`, and forty of them was a third of the entire CI test step.
  `src/apps/tower_defence/tests/CMakeLists.txt` sets the two seed counts from `ENABLE_COVERAGE`: eight seeds under instrumentation, which is measured to reach every line, function and branch of the generator, and forty otherwise.
  CI runs the wide sweep in the GNU leg's "Soak the level generator" step, which builds that one target against `conan-release`.
  So the coverage legs prove the coverage and an optimised build proves the property, and neither pays for the other.
  Wall columns with one gap each keep the grid connected while forcing the path to weave, and `Tile::Empty` is symbol 0 because `wfc::Solver` tries candidates in ascending order.
  A tight per-attempt step budget with many reseeds beat one large budget by roughly twenty times: a hard seed is cheaper to abandon than to grind out.
  `td::Battle` is the simulation -- integer throughout, no clock and no global generator, so it is a pure function of the tick count and the state.
  A tower's target needs no tie-break: mobs are kept in spawn order and all advance one cell per tick, so no two ever share a path index.
  **The app defines no event for placing a tower**: a click is the input, `td::TowerPlacementSink` turns it into a placement inside the tick path, and the replay stores the click and regenerates the placement -- persisting both would build two towers per click.
  Rendering is a write-only projection in structure rather than by promise: `td::snapshotOf()` takes an immutable `td::BattleSnapshot`, `td::BattleScene` turns that into drawing calls, and `td::RenderSink` runs it once per `engine.tick`, registered after `BattleSink` and `ScoreSink` so a frame is of the state the tick ended with.
  The running score is drawn by `antwika::ui`, described by `td::ScoreSink` inside the tick path and painted from `td::ScoreOverlay`, so no `ui.*` event exists here either.
  `td::GridLayout.hpp` is the one place the pixel-to-cell mapping lives, shared by the scene and the placement sink so the board somebody sees and the board they can build on cannot drift.
  It reserves a strip along the top for the score bar and lays the grid out below it, which is why a click on the bar falls outside the grid and builds nothing -- no sink has to ask the UI whether it covered the pointer.
  That strip is `td::scoreBarHeight(canvas)` -- the height the bar itself lays out to, derived from the same theme padding and glyph line height -- rather than a fixed pixel count, which only matched the bar at one window size and left the bar covering grid rows at any taller one.
  It starts on an empty grid and loads nothing unless `--replay` says so, so what a session contains is what somebody clicked.
  `src/apps/tower_defence/replays/demo.json` is a sample session to pass to `--replay`.
- `apps/sudoku` is unrelated to the tick/replay system: it's a showcase for `antwika::wfc` (Wave Function Collapse) — a standalone, dependency-free, deterministic constraint solver operating on a flat, index-addressed `std::vector` of cells with geometry expressed entirely through `IConstraint`s (no grid concept inside the library).
  `apps/sudoku` expresses the 81-cell puzzle and its row/column/box rules as `AllDifferentConstraint`s over that flat array — see [`blog/005-wave-function-collapse-that-never-guesses.md`](blog/005-wave-function-collapse-that-never-guesses.md).

**Supporting libs**: `antwika::time` (fixed-tick `Tick` type, `IClock`/`SystemClock`) and `antwika::log` (`ILogger`/`Logger`, `IAppender`/`IFormatter`/`ILogPolicy` — composable logging with no global state) are used across apps but carry no tick/replay logic of their own.
`antwika::ecs_commons` is the vocabulary half of the ECS -- `GridPosition`, `Velocity`, `Lifetime`, `Name`, `Tag<Kind>` and the `MovementSystem`/`LifetimeSystem`/`PeriodicSystem` that act on them -- kept out of `antwika::ecs` because that library is the mechanism and these are content, so an app wanting the scheduler does not link a countdown it never uses; see [`docs/ecs-commons.md`](docs/ecs-commons.md).

`antwika::pathfinding` is an A* that knows nothing about grids: the world arrives through an `IGraph` supplying `neighbours()` and `heuristic()`, and no cell, coordinate or extent appears in the core — `GridGraph` is a 4-connected convenience layered on top for the callers that do have a grid.
A missing path is `SearchOutcome::NoPath`, an ordinary answer rather than an exception, so `PathfindingError` is left for genuine precondition breaches like a negative edge cost.
The open set orders on estimated total cost, then remaining estimate, then ascending `NodeId`, and that third key is what makes the order **total**: no two entries ever compare equivalent, so the heap never gets to choose and an equal-cost route resolves the same way on every run and every toolchain — which is the only reason a replay may depend on a path at all.
See [`docs/pathfinding.md`](docs/pathfinding.md).

`antwika::animation` resolves which frame to show and holds no time of its own: a `Clip` is keyframes plus a loop policy, and `resolve(clip, elapsedTicks)` is a pure function of the tick the caller already has.
There is deliberately no `Animator` you advance — that would be simulation state hidden in a renderer, and a renderer calling `advance()` is the replay-drift bug that looks fine live.
It depends on `antwika::time` and nothing else, so it cannot name a texture or a rectangle: a `Frame` is an index the app maps to an atlas slot, and sub-tick position is an exact rational `Progress` rather than a float.
See [`docs/animation.md`](docs/animation.md).

`antwika::i18n` is a message catalogue keyed by a symbolic `MessageId` rather than by the English string, which is what lets a test assert that every locale covers **exactly** the same id set — a missing translation fails the build instead of leaking English into a Swedish UI.
Lookup is total and never throws, since it runs while a frame is being drawn: active catalogue, then the default locale, then the id's own name in exclamation marks, with every result carrying an `Exact`/`Fallback`/`Missing` origin so a caller can tell which of the three it got.
Catalogues are compiled in rather than loaded, so the library opens no files.
See [`docs/i18n.md`](docs/i18n.md).

`antwika::sound` decodes PCM audio, mixes it, and plays it through a build-time backend seam exactly as `gfx` and `input` have (`ISoundBackend`/`IDevice`/`IRenderCallback`, `SoundError`, `makeSelectedSoundBackend()`), so no code under `src/` names a concrete audio framework.
**It owns no thread, no lock and no queue**: a device renders only when `pump()` asks it to, on the thread that asked, which is why a headless run costs no wall-clock time and the whole suite runs with no sound card.
The usual arrangement -- a framework's own high-priority callback thread with a lock-free queue feeding it -- would be a second concurrency model in a codebase that has none, so `SoundCapabilities::selfDriven` is how a backend that genuinely cannot be pumped says so, and the conformance suite skips those tests rather than failing an honest backend.
**A callback is handed the *absolute* index of the first frame it fills**, counted from the start, never a count since the last call -- which is the one interface decision that would be expensive to undo and the thing real devices most often get wrong, since a per-buffer counter puts every scheduled sound on the wrong frame after a single dropped buffer.
That is what makes `PlayRequest::startFrame` mean something: a sound placed at frame 48,000 begins there rather than at the next buffer boundary, so "play it now" is deliberately not expressible.
`framesPlayed()` is monotonic and **advisory** -- legal to read to decide how long to sleep, never to decide what to compute, since it is the one number a real device derives from hardware and hardware does not agree with a tick count.
A `Waveform` is always normalised float whatever width the file held, decoded once to a plain value as `gfx::Bitmap` is, so there is no sample-format enum and no conversion matrix anywhere; float is unarguable here because audio is a write-only projection in exactly rendering's sense.
`SampleBuffer` is planar and non-owning so `Mixer::render()` allocates nothing, `Waveform` is interleaved because that is what a file holds, and `OfflineDevice` is the one place the two layouts meet.
`NullSoundBackend`, `NullDevice` and `OfflineDevice` live *in* the library rather than under `backends/`, following the `NullInputBackend` precedent, which is what puts them inside the coverage gate.
`WavReader::read()` is hand-rolled and takes a `std::istream` rather than a path, for `PngReader`'s two reasons: the library opens no files, and every refusal it can produce is reachable from bytes in memory.
A waveform whose rate differs from the mixer's is refused rather than played at the wrong speed, since this library does not resample.
It depends on `antwika::log` and nothing else -- not `time`, not `ecs`, not `replay` -- and holding no clock is what leaves room for a musical layer above it, exactly as `antwika::animation` holding none does.
See [`docs/sound.md`](docs/sound.md).

`antwika::input` abstracts reading a keyboard and a pointer (`IInputBackend`/`InputEvent`/`InputCapabilities`, `InputError`), so no code under `src/` names a concrete input framework; backends live under [`backends/`](backends/) beside the graphics ones and are chosen at build time by `ANTWIKA_INPUT_BACKEND`.
It deliberately does **not** depend on `antwika::gfx`, and `antwika::gfx` does not depend on it — reading input does not require opening a window, which is why `input::Position` duplicates `gfx::Point` rather than reusing it, and why an input event does not say which window it arrived at.
**That rule is about the source, not the link line**: no file under `src/libs/input` includes a `<antwika/gfx/...>` header or names a `gfx::` type, and that is what it forbids.
`antwika::gfx` is nevertheless *on* that link line, transitively: `antwika::replay` links `antwika::ecs` and `antwika::gfx` for `TickPacer` and `WindowInputSource`, and `antwika::input` links `antwika::replay` because every source it offers is an `IReplaySource`.
That was reviewed and accepted rather than overlooked — a linker seeing a library is not a dependency in the sense the rule cares about, since nothing in `input` can call into it — so a reader who finds `gfx` in `antwika_input`'s transitive link set has not found a violation.
Every `InputEvent` is an *edge* (a press, a release, a move, a notch) and never a statement of what is currently held, which is what lets a queue-based framework (SDL) and a state-polling one (raylib) implement the same interface.
Live input reaches the engine only through `IReplaySource`, via `LiveInputSource`, and is persisted by `InputEventCodec` as `input.*` events with symbolic key and button names rather than platform scancodes, so a session recorded under one backend replays under another; no `input.*` name may ever be added to an app's `kSelfGeneratedEventNames`.
What a recording holds is thinned by two decorators that sit *upstream* of `TickEventRecorder`, which is the only place a reduction may happen: doing it after the recorder would make the file disagree with the run that wrote it, and doing it in a backend would hide it behind the seam.
That rule governs the event stream, which is everything a replay has to reproduce, and there is now exactly one thing deliberately outside it -- `input::PointerHintChannel`, two paragraphs down.
`CoalescingPointerSource` keeps only the last of each run of movements inside a tick; `IdleMotionSource` holds back movement that arrives while no button is held, latching the last of it and releasing it immediately ahead of the first event that could read a position -- since `input.pointer_scroll` carries none of its own and a zoom anchors on the folded one.
Which of the two an app may attach is an app-level question: `game` takes both, `life` takes only the gate, because a drag toggles every cell it crosses and thinning a run inside a tick would skip some.
Both decorators exist because a window system reports pointer motion at its own rate rather than the app's -- SDL will report several hundred movements a second into a run that ticks 25 times a second -- so an unthinned `--record` file grows at the window system's rate and is mostly positions nothing ever read.

**A free-moving pointer reaches an app on a channel that is not an event and is in no recording.**
`input::PointerHintChannel` holds one `input::PointerHint` -- where the pointer is -- written once per tick by `input::PointerHintSource` and read by whatever draws, through an accessor named `forRenderingOnly()`.
An app opts in by naming a channel in `InputPipelineOptions::pointerHint`; naming none attaches nothing, so every existing app records byte for byte what it recorded before.
**What is read off that channel may decide what is drawn, and nothing else** -- that is the entire safety condition, and it is the price of the channel existing.
A live run and its replay do not agree on the value, deliberately: a replay holds none of the motion between clicks, so replaying publishes only the positions its recorded events happen to carry.
Fold a hint into anything a replay reproduces and the run and its replay diverge, silently, with the symptom nowhere near the line that caused it.
**That it is a value cell rather than an event carrying a "do not record" marker is what makes that structural rather than remembered.**
A marked event would still travel the dispatcher, so every `ITickEventSink` an app owns would be handed it and the rule would become a thing each sink has to observe; a value cell reaches a sink only if somebody passed that sink the channel in a `main.cpp`, in the open, next to the pipeline that publishes it -- the move `game::UiOverlay` and `life::DragState` already make.
`PointerHintSource` is a pure observer: `eventsFor()` returns its inner source's events unmodified, so a recording is a function of a stream it cannot touch, which is what makes attaching it free rather than merely cheap.
`InputPipeline` attaches it immediately outside `LiveInputSource`, so no thinning decorator can hide a movement from it, and it is attached on a replay run too so the two branches still differ only in whether a device is read.
**The channel does not make `IdleMotionSource` redundant, and the gate does not make the channel redundant**: the gate thins the recording and publishes nothing, the channel publishes and thins nothing, and an app that draws a hover wants both -- which is a combination that was not previously available, since the gate's one documented cost was that a hover could not be drawn at all.
The consequence to hold in mind is that the channel runs *ahead* of the event stream -- on the tick a gated movement arrives the channel already has it, while the stream will not carry it until the next press, wheel or key -- which is the point of the channel and also exactly why the two may never be mixed.
Because SDL drains one process-global queue for windows *and* input, `backends/sdl3` owns a reference-counted `Sdl3Pump` shared by both of its targets, which calls `SDL_PollEvent` once and routes each event into a window queue or an input queue.
That sharing belongs in `backends/` rather than in `src/` because a framework directory already owns that framework's global state: admitting the single queue in one place behind the abstraction keeps the two library seams independent, where lifting it into `src/` would make it a rule `antwika::gfx` and `antwika::input` had to cooperate on.
The `raylib` input backend reports a pointer and no keyboard, and synthesises edges by diffing state, since raylib has no queue at all.

`antwika::gfx` abstracts opening and rendering to windows (`IGfxBackend`/`IWindow`/`IRenderer`, `GfxError`), so no code under `src/` names a concrete graphics framework — SDL, raylib and friends arrive as statically linked backends under `backends/`, chosen at build time.
Rendering is a write-only projection of state and never feeds back into the tick loop, so replays stay reproducible under the headless `NullBackend`.
See [`blog/012-a-window-that-cant-talk-back.md`](blog/012-a-window-that-cant-talk-back.md) for how an app hangs rendering off the tick loop without letting it feed back in.
A window may be resizable, and the two sizes it then has are deliberately named apart: `IWindow::configuredSize()` is the size the app asked for and is the same number on the recording and the replaying machine, while `IWindow::size()` is what the window currently reports.
**Nothing in a simulation may be driven from the reported size** — laying out or hit-testing against it would make a window resize change what a recorded click means — so it is only ever used to place what is drawn inside the drawable area; see [`docs/resizable-windows.md`](docs/resizable-windows.md).

Textures are decoded once and uploaded per backend: `gfx::PngReader::read()` turns a byte stream into a `gfx::Bitmap` of straight RGBA (stb_image, compiled `STB_IMAGE_STATIC` in one TU because raylib links its own copy), `IRenderer::createTexture()` uploads it, and `drawTexture(texture, source, destination, tint)` blits part of it with a colour and alpha modulation.
The library opens no files — an app does that, as `apps/gfx_demo` does with `app::assetPath()`, which finds the PNG shipped in the application's own directory under `bin/`.
A texture belongs to the renderer that made it: drawing it through any other draws nothing, and it may safely outlive its window, because each renderer's `detach()` frees its live textures before the framework tears the device down.
Write-only still holds — `ITexture` is opaque, and there is no pixel read-back, render target or screenshot anywhere in the interface.

**3D is a sibling interface, not more methods on `IRenderer`.**
`gfx::IRenderer3D` (`createMesh()`, `drawMesh(mesh, model, camera, tint)`) is reached through `IRenderer::renderer3d()`, which is non-pure and returns null by default: a backend with no 3D path says so rather than accepting a draw and dropping it, and every existing implementer of `IRenderer` — backends and test doubles alike — kept compiling unchanged.
`gfx::IMesh` mirrors `ITexture` exactly (opaque, owned by the renderer that made it, no read-back of any kind), and `clear()`/`present()` stay on `IRenderer` because there is one frame that both halves draw into.
The maths is GLM, aliased rather than wrapped in `Math3D.hpp` (`Vec3`, `Mat4`), with `gfx::Transform` and `gfx::Camera3D` (perspective or orthographic) on top.
**Those types are render-side only**: they are floating point, and floating point may never appear in anything a replay reproduces — which costs nothing, because rendering is already a write-only projection.
`apps/game`'s camera is the opposite case and is *not* one of these: it is simulation state, because a click's meaning depends on it, which is exactly why it holds whole tile sizes rather than a scale factor.
`null` and `raylib` implement `IRenderer3D`; `sdl3` inherits the null default and reports no 3D renderer, which is a conforming answer rather than a gap.
The raylib one sets the view and projection matrices through `rlgl` (`rlSetMatrixProjection`/`rlSetMatrixModelview`) rather than handing raylib a `::Camera3D` to `BeginMode3D()`.
That struct describes a projection by a field of view and picks its own clip planes, so a `gfx::Camera3D`'s near and far planes -- and an orthographic one's extents -- would be discarded and quietly replaced; nothing would fail and the scene would simply be wrong.
`RaylibMesh` copies `RaylibTexture`'s ownership rules exactly, and `RaylibMaterial` wraps raylib's default material, which `DrawMesh` insists on being handed and which the tint is set on.
raylib indexes a mesh with 16-bit indices where `MeshData` says 32, so a mesh with more vertices than one of those can address is refused with a `GfxError` rather than silently wrapped around.
The shared conformance suite covers the 3D calls too, and every one of those tests skips when a backend offers no 3D renderer.
`apps/gfx3d_demo` (`antwika_gfx3d_demo`) is the showcase: a cube whose turn is a function of the **tick count** and never of a clock, drawn through `IRenderer3D` with the caption drawn over it through `IRenderer` -- one frame, both halves.
It draws a fixed number of frames rather than running until closed, since the default `null` build reports no close and every CI leg produces it.

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
Once focus is in play — the caller passed some in, or sent a key — a pointer press moves focus to whatever it activated, so the ring and the keystrokes cannot end up on different widgets; a caller using the pointer alone never has focus in play and so never gains a ring it did not ask for.
Enter reports through `Interactions::activated` exactly as a press does, so one code path handles both.
The focused widget draws `Theme::focusRing` (yellow) `Theme::focusRingThickness` pixels thick as four `FillRect`s appended *after* every widget, since `IRenderer` has no stroke and a container declared later would otherwise paint over a ring drawn in place.
**A text field and a dropdown hold nothing of their own either**, which is the same rule read out loud: a field's characters and caret arrive in `ui::TextFieldSpec`, a list's open/closed and selected state in `ui::DropdownSpec`, and what happened comes back as `ui::Interactions::edit` (a `ui::TextEdit`) and `ui::Interactions::chosen` (a `ui::OptionChoice`).
The application owns all of it, so a replay regenerates it from the recorded input rather than from anything the UI remembered.
Typing arrives on the same `ui::Keyboard` the focus keys do -- `ui::Key::Backspace`/`Cancel`/`MoveLeft`/`MoveRight` beside `FocusNext`/`FocusPrevious`/`Activate`, plus a `typed` view of the characters -- rather than as a second input channel, and `TextFieldSpec::focused` is an override on top of the focus `Context` was handed, so Tab reaches a field and Enter submits the one it landed on.
An open dropdown's list is an *overlay*: out of its parent's flow, hung beneath the box it dropped from, painted after every other command and hit-tested before them -- which is the only way to be on top when `antwika::gfx` offers no depth but paint order.
**Where a widget ended up is the third answer off the one layout**: `Frame::rects` is a `ui::WidgetRects`, one `gfx::Rect` per distinct `ui::WidgetId` the frame named, with `find(id)` answering nothing for an id this frame did not declare.
It exists so an application drawing its own art around a UI places that art *from* the layout rather than beside it -- two independently computed layouts agree only until either one changes, which is precisely how `apps/poker`'s card art and its `antwika::ui` labels came to disagree.
`ui::ContainerSpec` therefore carries an `id` as well, so a row or a panel can be named; that also makes it something the pointer reports as hovered or activated, since that is the one thing an id means here, and a child sits at a higher index so it still wins the hit-test against the container holding it.
Every named node answers rather than only containers, because a node carries one id whatever kind it is and a button's rectangle is as useful to something drawing behind it as a row's is.
The mapping is collected *inside* the arranging pass rather than by a pass of its own, so the rectangle reported is the one `flatten()` drew from by construction -- including under the proportional shrink a container with too little room applies, which is exactly where a repeated sum would diverge.
A repeated id keeps its last declaration, following the existing rule that two nodes sharing an id are one widget.
A caller that names nothing pays one integer comparison per node and a vector that never allocates, and reading a rect back is safe anywhere including inside the tick path, because a layout is a pure function of the declarations, the theme and the canvas -- which is what makes it unlike `input::PointerHintChannel`.

## Notes for AI agents

- **Always work in a separate git worktree, never directly in the primary checkout.** Before making any change, create/enter a dedicated worktree for the task (`git worktree add .worktrees/<task> -b <task>`), do all editing, building, and testing there, and only merge back when the work is done.
  This keeps `main` clean and lets several tasks build in parallel without clobbering each other's `build/` directory.
  `.worktrees/` is the agreed home for them and `.gitignore` covers it, so a worktree and its build output never show up as untracked state in the primary checkout; `.claude/` and `core.*` are ignored for the same reason.
- The blog posts under `blog/` are design write-ups for *why* a piece was built the way it was, written after the fact — read the relevant one before changing a library's core abstraction, since it usually explains a constraint that isn't obvious from the code alone.
- Prefer running a single test binary (or `--gtest_filter`) over the full `ctest` suite while iterating; run the full suite before considering a change done.
