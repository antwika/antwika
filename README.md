[![CI](https://img.shields.io/github/actions/workflow/status/antwika/antwika/ci.yml?branch=main&style=plastic&label=CI)](https://github.com/antwika/antwika/actions/workflows/ci.yml)
[![Coverage (GNU)](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/antwika/antwika/badges/coverage-gnu.json&style=plastic)](https://github.com/antwika/antwika/actions/workflows/ci.yml)
[![Coverage (LLVM)](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/antwika/antwika/badges/coverage-llvm.json&style=plastic)](https://github.com/antwika/antwika/actions/workflows/ci.yml)
[![License: Apache 2.0](https://img.shields.io/badge/License-Apache%202.0-blue.svg?style=plastic)](LICENSE)

A C++23 game project built with CMake, Conan, and GoogleTest, developed inside VS Code Dev Containers for a fully reproducible toolchain across Linux (GNU/LLVM) and Windows (MinGW).

## Wiki

[`wiki/Home.md`](wiki/Home.md) is the project wiki: a page per library and per app, plus architecture, getting-started, contributing and glossary pages.
It is plain markdown with relative links, so it reads on GitHub and in an editor with no tooling.

## Project structure

```
src/
├── apps/
│   ├── game/
│   ├── gfx_demo/
│   ├── life/
│   ├── poker/
│   ├── sudoku/
│   └── task_worker/
└── libs/
    ├── ecs/
    ├── engine/
    ├── event/
    ├── gfx/
    ├── holdem/
    ├── input/
    ├── log/
    ├── replay/
    ├── scheduler/
    ├── time/
    ├── ui/
    └── wfc/
backends/
├── null/
├── raylib/
└── sdl3/
blog/
```

Each library and app has its own `CMakeLists.txt`, `include/`, `src/`, and `tests/` directory.
`backends/` sits outside `src/` and holds the concrete graphics and input frameworks, one directory per framework, exactly one of which is compiled into a given build.
See [`docs/STYLE_GUIDE.md`](docs/STYLE_GUIDE.md) for the project's C++/CMake/Python coding conventions.

`blog/` holds write-ups about notable changes to the project — see [`blog/001-building-a-deterministic-replay-system.md`](blog/001-building-a-deterministic-replay-system.md) for the design and requirements behind the replay system below, and [`blog/003-an-entity-component-system-with-nowhere-to-hide-a-mutation.md`](blog/003-an-entity-component-system-with-nowhere-to-hide-a-mutation.md) for the `antwika::ecs` library under `libs/ecs/`.

## Quick start

Open the project in **Visual Studio Code**, then reopen it in a development container:

```
Ctrl + Shift + P > Dev Containers: Reopen in Container
```

When prompted to choose a container, select one of the following:

- **GNU Dev Container**
- **LLVM Dev Container**
- **MinGW Dev Container**

The **Base Dev Container** provides only the common development environment used as the foundation for the other containers.
It is not intended for regular development or for building the project directly.

> When switching between different dev containers, you may need to remove the `build` directory first.

Build and test the project using:

```
Ctrl + Shift + B
```

After the build completes, run the compiled binaries on your target machine:

- Linux: `build/bin/antwika_game`, `build/bin/antwika_life`, `build/bin/antwika_poker`, `build/bin/antwika_sudoku`, `build/bin/antwika_task_worker`
- Windows: `build/bin/antwika_game.exe`, `build/bin/antwika_life.exe`, `build/bin/antwika_poker.exe`, `build/bin/antwika_sudoku.exe`, `build/bin/antwika_task_worker.exe`

### Choosing a graphics and input backend

Builds use the `null` graphics and input backends, which open windows that draw nothing, report no input, and need no display.
To build against a real one, pick it once:

```
Ctrl + Shift + P > Tasks: Run Task > Select gfx backend
```

Choose `null`, `sdl3` or `raylib`.
`Ctrl + Shift + B` builds that backend from then on, so the choice is made once rather than on every build.
The same thing works from a terminal:

```sh
scripts/select_gfx_backend.sh sdl3
scripts/build.sh
```

A real backend builds into `build-sdl3/` or `build-raylib/` rather than `build/`, matching what CI does, so switching backends never invalidates the previous one.
The choice drives input as well as graphics: the `input_backend` Conan option and the `ANTWIKA_INPUT_BACKEND` CMake variable both default to whatever was picked for graphics, so `sdl3` windows come with `sdl3` keyboard and mouse.
Setting them apart is allowed for input with no window, or a window with no input:

```sh
conan install . -of build-sdl3-input -o gfx_backend=null -o input_backend=sdl3 ...
```

Naming two different real frameworks is refused at configure time, because they would fight over one operating-system event queue and whichever polled second would silently lose events.
`build/bin/antwika_gfx_demo` opens a window and draws until you close it -- under the `null` backend there is nothing to close, so that build runs until interrupted.
It draws three bars and blits a PNG logo twice: once whole and untinted, once left-half-only and tinted, which is what a source rectangle and a tint look like side by side.
`build/bin/antwika_gfx3d_demo` is its counterpart for the 3D half: a cube drawn through `gfx::IRenderer3D`, turned by the tick count rather than by a clock, with a caption drawn over it through the 2D calls.
It stops after a fixed number of frames, because the `null` backend reports no close and that is the build every CI leg produces.
The selection lives in the untracked `.vscode/gfx-backend`, which makes it yours rather than the repository's.

## Replays

The engine runs on a fixed timestep and every event dispatched during a run is tick-stamped, so a run can be recorded and later reloaded to reproduce the exact same resulting state:

```sh
build/bin/antwika_game                        # empty grid, runs until you quit
build/bin/antwika_game --record demo.replay   # the same, saving what you did
build/bin/antwika_game --replay demo.replay   # reload it, reproducing the run
```

Both modes go through the same `antwika::game::bootstrap()` entry point and the same fixed-timestep tick loop (`antwika::replay::EngineLoop`) — replay mode only differs in where each tick's events come from.
`apps/game` itself is an isometric grid you build on with the mouse: left-click lays a path, right-click drops a walker onto it, middle-drag pans and the wheel zooms.
Walkers advance a cell every second tick, preferring a right turn at an intersection and reversing at a dead end.
The ground, the roads and the walkers are all blitted from one texture atlas (`src/apps/game/assets/atlas.png`), so the scene draws no shape of its own: the grid lines are painted into the ground tile's own edges, and a road's sixteen tiles are addressed by which neighbours it joins, which makes a junction a lookup rather than four stubs stepped out by hand.
That picture is drawn by `scripts/generate_game_atlas.py` from the same slot numbers `antwika/game/TileAtlas.hpp` addresses it with, and CI fails if the committed one has drifted from the generator.
It starts empty and loads nothing unless `--replay` asks it to, and runs until you press Escape or close the window -- both of which are input, so both end up in a recording.
Under the `null` backend neither is available, so that build runs until interrupted; `src/apps/game/replays/demo.json` is a sample session to feed `--replay` if you want to watch one without a mouse.
What a recording holds is the clicks, not what they caused: the app defines no event for placing a path, so a replay stores the click and regenerates the placement rather than persisting both.
Application code defines its own state (`GameState`) and events (e.g. `game.score_increment`) on top of the engine's built-in per-tick event (`engine.tick`), both reacted to through the same `ITickEventSink` mechanism — see [`blog/001-building-a-deterministic-replay-system.md`](blog/001-building-a-deterministic-replay-system.md) for the full design and how to add your own.

`apps/life` is a second, independent application built on the same replay system, this time with its state held in an `antwika::ecs::World` instead of a plain struct — a Conway's Game of Life board, where each cell is an entity with a `Cell` component and a single `LifeSystem` advances every cell one generation per tick using the double-buffered `World`/`SystemScheduler` machinery described in [`blog/003-an-entity-component-system-with-nowhere-to-hide-a-mutation.md`](blog/003-an-entity-component-system-with-nowhere-to-hide-a-mutation.md):

```sh
build/bin/antwika_life                        # seeds a glider, then runs on
build/bin/antwika_life --record demo.replay   # save the input as a replay
build/bin/antwika_life --replay demo.replay   # reload it, reproducing the run
```

Cells are toggled alive via a `life.toggle_cell` event (JSON payload `{"x":..,"y":..}`), tick-stamped exactly like `game.score_increment` — the same event-driven, replayable pattern applied to ECS state instead of a hand-rolled reducer.

The run has no end of its own: it goes on until the window is closed, or until a replay says to stop.
A headless build reports neither, so `Ctrl+C` is what ends one.

It is also where `antwika::gfx` and `antwika::input` earn their keep.
Built against a real backend, `antwika_life` draws the board into a window instead of printing it, one frame per tick, and lets you draw on that board with the mouse:

```sh
scripts/select_gfx_backend.sh sdl3 && scripts/build.sh
build-sdl3/bin/antwika_life                   # a glider crossing a 32x32 board
```

Drag with the left button held to toggle every cell the pointer crosses, one toggle per cell per drag, and watch the next generation take it from there.
Holding the button also pauses the simulation, so the board stays still while you draw on it rather than evolving out from under the cursor; the cells you toggle still appear as you draw them, and the generations pick up again when you let go.
Under the default `null` backend there is no window to draw into, so that build prints the board as ASCII instead, which is what keeps the app runnable in CI with no display present.
Drawing is a write-only projection of the `World` and never feeds back into it.
Closing the window enters the engine as an `engine.stop` event through the same `IReplaySource` every other external input goes through, so a run ended by closing a window is recorded like any other input — and replaying that recording headlessly reaches the identical board.
The mouse arrives the same way, as `input.pointer_down`/`input.pointer_move`/`input.pointer_up` events: what a `--record` run persists is the click, and which cell it toggled is derived from it again on replay.

`apps/task_worker` is a third application, this time combining `antwika::ecs` with a new `antwika::scheduler` library: a fixed pool of `Worker` entities pulls tasks off a deterministic, priority-ordered, budget-bounded `antwika::scheduler::Scheduler`, submitted over time via a `task.submit` event and, optionally, chained to an earlier task with a dependency edge:

```sh
build/bin/antwika_task_worker --record demo.replay   # submits a mixed-priority task burst
build/bin/antwika_task_worker --replay demo.replay   # reload it, reproducing the same run
```

Tasks are submitted via a `task.submit` event (JSON payload `{"id":..,"priority":..,"durationTicks":..,"label":..}`, plus an optional `"dependsOnId"`), tick-stamped exactly like `life.toggle_cell` — a `TaskSubmissionSink` schedules each parsed task onto the `Scheduler`, and a `TaskDispatchSystem` runs the scheduler each tick with that tick's idle-worker count as its budget, so no more tasks start than there are free workers.
See [`blog/006-a-job-scheduler-and-a-worker-pool-that-cant-lie-to-itself.md`](blog/006-a-job-scheduler-and-a-worker-pool-that-cant-lie-to-itself.md) for the full design.

## No-limit hold'em

`libs/holdem` (`antwika::holdem`) is a standalone poker library: hand evaluation, the betting rules, side pots, and the loop that asks each player what they want to do.

Its centrepiece is the evaluator.
A hand's strength collapses to a single 24-bit number where greater is stronger and equal means the two hands split the pot — there is no secondary tie-break anywhere, because every detail poker cares about is already inside the number.
Getting there is pure bit manipulation: four 13-bit rank masks, one per suit, folded together with shifts, ands and ors.
Duplicate ranks fall out of pairwise intersections of those masks rather than any counting, straights out of `m & m>>1 & m>>2 & m>>3 & m>>4` over a mask extended by one bit so the ace can also play low, and flushes out of a population count.
Nothing is enumerated and no five-card subset is ever materialised, so scoring seven cards costs the same as scoring five.

`apps/poker` (`antwika_poker`) is the showcase, and it is a fourth application on the same replay system:

```sh
build/bin/antwika_poker --record demo.replay   # a cash game, saving who bought in
build/bin/antwika_poker --replay demo.replay   # reload it, reproducing the same session
build-sdl3/bin/antwika_poker --tick-delay-ms 150   # or watch it, in a window
```

One engine tick is one step of the poker loop: dealing a hand, or asking a single player to act.
A hand runs pre-flop, flop, turn, river and showdown, and the next one is dealt as soon as the last is paid out.
Balances live outside the games in a `BankrollLedger`, and a `CashGame` is the only door between a bankroll and a seat — so a player can never buy in for more than they actually hold, and the total of every bankroll plus every stack is conserved across a whole session.

Every hand is written out in the hand-history layout trackers and replayers already read, so a session can be pasted straight into one:

```text
Antwika Hand #2: Hold'em No Limit (5/10) - 2026/07/30 07:34:17
Table 'Antwika' 6-max Seat #3 is the button
Seat 1: alice (370 in chips)
Seat 2: bob (400 in chips)
Seat 3: carol (430 in chips)
alice: posts small blind 5
bob: posts big blind 10
*** HOLE CARDS ***
Dealt to alice [Ac As]
carol: calls 10
alice: raises 25 to 35
bob: folds
carol: calls 25
*** FLOP *** [5h 2s 5c]
...
*** SHOW DOWN ***
alice: shows [Ac As] (two pair, Aces and Fives)
carol: shows [7c Js] (two pair, Fives and Deuces)
alice collected 240 from pot
*** SUMMARY ***
Total pot 240 | Rake 0
Board [5h 2s 5c Kh 2h]
Seat 1: alice (small blind) showed [Ac As] and won (240) with two pair, Aces and Fives
Seat 2: bob (big blind) folded before Flop
Seat 3: carol (button) showed [7c Js] and lost with two pair, Fives and Deuces
```

Every player's cards are dealt face up here, since a table of bots has nobody to keep them from.
See [`blog/011-writing-a-hand-history-the-rest-of-the-world-can-read.md`](blog/011-writing-a-hand-history-the-rest-of-the-world-can-read.md) for how the printer gets the numbers that format wants out of a table that never had to track them.

The same session also draws itself, through `antwika::gfx`: felt, the board, the pot, and one row per seat with its stack, its cards and a ring round whoever has to act.
`--tick-delay-ms <n>` is what makes that watchable, holding each tick's frame for `n` milliseconds and then keeping the final frame up until the window is closed; without it the ~250-tick demo is over in milliseconds.
Under the default `null` backend the window draws nothing, so the terminal run is exactly what it always was.
A real backend needs a display — `SDL_VIDEODRIVER=dummy` or `xvfb-run` otherwise.

Closing the window ends the session, and it does so as a `engine.stop` fed in through the `IReplaySource` the loop already reads from, never by reaching into the loop.
That is what keeps drawing a write-only projection: a windowed run reaches the same chip counts as a headless one, and a session ended by closing the window replays under `null` to the same result.

Money moving in and out is all a replay stores: `poker.deposit`, `poker.buy_in` and `poker.cash_out` events (JSON payloads `{"player":..,"amount":..}`).
Not one card and not one action is recorded, because the shuffle is seeded from `RoomConfig` and the agents behind `antwika::holdem::IAgent` are deterministic functions of what they are shown — so a reloaded session deals the same cards and reaches the same chip counts by construction.
See [`blog/010-a-poker-hand-in-one-number.md`](blog/010-a-poker-hand-in-one-number.md) for the full design, including the short-all-in rule the tests turned up.

## Wave Function Collapse and Sudoku

`libs/wfc` (`antwika::wfc`) is a standalone, dependency-free constraint-solving library implementing Wave Function Collapse: repeatedly pick the lowest-entropy cell, collapse it to one candidate value, propagate the consequences, and backtrack on contradiction.
It is deterministic (no RNG, fixed tie-breaks) and complete: an exhaustive backtracking search distinguishes a proven-`Unsatisfiable` puzzle from one that merely ran out of an optional step budget (`LimitExceeded`).
It also scales to large waves via worklist-driven propagation and a trail-based undo log, rather than copying the whole wave per branch.
Its own data model is a flat, index-addressed `std::vector` of cells with no notion of a grid.
Geometry is entirely up to the caller, expressed as `IConstraint`s over cell indices.

`apps/sudoku` (`antwika_sudoku`) is the showcase: it expresses a Sudoku puzzle as an 81-cell wave and its row/column/3x3-box rules as 27 `AllDifferentConstraint`s over that flat array, then hands both to `antwika::wfc::Solver` — no 2D-grid code inside the library at all.

```sh
build/bin/antwika_sudoku                          # solves a built-in demo puzzle
build/bin/antwika_sudoku --puzzle my-puzzle.txt    # solves a puzzle loaded from a file
```

A puzzle file is 81 characters (whitespace ignored) of digits `1`-`9` or a blank marker (`.` or `0`).

## Immediate-mode UI

`libs/ui` (`antwika::ui`) lays out and draws nestable layouts, labels and buttons on top of the four calls `antwika::gfx::IRenderer` offers.
The caller writes immediate-mode code, and what that code builds is a tree, laid out only once it is finished:

```cpp
ui::Context ui{canvas, ui::scaledTheme(ui::Theme{}, ui::scaleForCanvas(canvas))};

{
    const auto screen = ui.panel({.height = ui::kGrow});

    ui.label("Antwika UI");

    {
        const auto side = ui.panel({.width = ui::kFit, .height = ui::kGrow});

        ui.label("layouts", ui.theme().muted);
    }
}

const auto frame = ui.finish();

paint(renderer, frame.commands);
```

Deferring the layout is what makes nesting work: a container cannot size itself from children it has not seen yet, so a one-pass design can only nest when the caller has already worked out every number.
Measuring runs backwards over the tree and arranging runs forwards, both as flat loops rather than recursion, so there is no nesting depth to exceed.

A container is opened by a `[[nodiscard]]` scope guard and closed when that guard goes out of scope.
There is no `end()` of any kind, so a mis-nested layout is not something the API can express.

`finish()` returns a `Frame`: a `DrawList` — a plain vector of fill and text commands — and what the pointer did to the widgets.
`paint()` turns the commands into renderer calls.
Keeping the picture as a value is what lets a whole layout be compared against an expected one in a test with no renderer, no window and no graphics framework involved.
`paint()` neither clears nor presents: a UI is drawn over whatever is already there, and whoever owns the frame decides when it is done.

A button can be clicked.
Name it in its spec and it works out its own hovered and pressed appearance, and `finish().interactions` reports which one a press landed on:

```cpp
constexpr ui::WidgetId kOk{1};

ui::Context ui{canvas, theme, pointer};

ui.button("ok", {.id = kOk});

if (ui.finish().interactions.activated == kOk)
{
    // ...
}
```

The pointer arrives as an argument rather than from a device, so the library still depends on `antwika::gfx` and nothing else — an application folds `antwika::input`'s edges into an `InputState` and hands the result across as a value.
Everything is resolved against the layout of the frame being drawn, so what a press hit is what was on screen when it was pressed.

Nothing is retained between frames: a widget activates on the press rather than on a release matched to it, which is what keeps the whole library a pure function of its declarations, its canvas and its pointer.
A button can still be told how to look, for the application that knows which one is in play.

An application drives all of this from inside its tick loop, downstream of the replay recorder, so a recorded click regenerates the button press rather than the press being recorded as well — `apps/game`'s `UiSink` is the worked example.

`apps/gfx_demo` (`antwika_gfx_demo`) is the showcase: a header, a sidebar sized from its own longest label, a growing main column, and a row of buttons pushed to the bottom right by growing spacers.
The buttons work: one counts your clicks and the other puts the count back to zero.

```sh
build/bin/antwika_gfx_demo    # needs a display; use xvfb-run without one
```

## Testing

Tests are built with GoogleTest and registered with CTest.
From the `build` directory:

```sh
ctest --output-on-failure
```

A helper script checks for unused test doubles (mocks/fakes that no `.cpp` file includes):

```sh
python3 scripts/check_unused_test_doubles.py
```

Another checks that comments and markdown prose keep to one sentence per line -- however long, but never two sentences sharing a line or one sentence wrapped across several:

```sh
python3 scripts/check_one_sentence_per_line.py
```

### Coverage

The GNU and LLVM toolchains build with instrumentation via the `conan-coverage` CMake preset, which configures into its own `build-coverage/` directory (separate from `build/`) so switching between a regular and a coverage build never leaves stale, uninstrumented object files behind.
Report line coverage with `gcovr`:

```sh
cmake --preset conan-coverage
cmake --build build-coverage -j24
ctest --test-dir build-coverage
gcovr --root . --filter 'src/.*' --exclude '.*/tests/.*' --print-summary build-coverage
```

CI runs this on every push to `main` for the GNU and LLVM toolchains (not MinGW, which doesn't support `--coverage`) and publishes the resulting percentage as the badges above.

### Badges

Coverage badges report **line / function / branch** coverage.
For example, **95% / 80% / 75%** indicates 95% line coverage, 80% function coverage, and 75% branch coverage.

The LLVM badge's branch percentage in particular runs noticeably lower than GNU's, and that gap isn't a real coverage hole.
`gcovr` passes `--exclude-throw-branches` to strip compiler-generated exception-unwind branches (the implicit edge taken if a call throws right before a non-trivial local destructor runs) from the count, but that only works because GCC's `gcov` tags those branches `(throw)`.
LLVM's `llvm-cov gcov` emulation never emits that tag, so the same flag is a silent no-op on the LLVM leg, leaving those unwind edges counted as "missing" branches.
Treat the two branch numbers as not directly comparable, and don't chase the LLVM one down to match GNU's via more tests — most of the gap can't be closed without injecting failures into unrelated calls (allocators, `fputs`, etc).

**The GNU badge is the one this project strives to bring to 100%.**
Its branch count reflects only this project's own logic, since GCC's `gcov` tags let `--exclude-throw-branches` do its job.
LLVM's can't reach 100% by design, so use GNU as the completion signal and treat LLVM's as informational.
See [`docs/confirming-unreachable-branches.md`](docs/confirming-unreachable-branches.md) for the procedure to follow before marking any remaining gap `GCOVR_EXCL_LINE` rather than writing a test for it.

## Optional: Use a locally built `antwika-dev-base` development container

Build the base development container locally:

```sh
docker build --no-cache \
  -t antwika-dev-base:latest \
  -f .devcontainer/base/Dockerfile .
```

Update the GNU, LLVM, and MinGW container definitions to use the locally built image:

```sh
sed -i 's|FROM ghcr.io/antwika/antwika-dev-base:${BASE_VERSION}|FROM antwika-dev-base:${BASE_VERSION}|' \
  .devcontainer/gnu/Dockerfile \
  .devcontainer/llvm/Dockerfile \
  .devcontainer/mingw/Dockerfile
```

VS Code will then use the locally built `antwika-dev-base` image when creating the GNU, LLVM, or MinGW development containers.

## License

Licensed under the [Apache License 2.0](LICENSE).
