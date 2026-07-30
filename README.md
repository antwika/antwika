[![CI](https://img.shields.io/github/actions/workflow/status/antwika/antwika/ci.yml?branch=main&style=plastic&label=CI)](https://github.com/antwika/antwika/actions/workflows/ci.yml)
[![Coverage (GNU)](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/antwika/antwika/badges/coverage-gnu.json&style=plastic)](https://github.com/antwika/antwika/actions/workflows/ci.yml)
[![Coverage (LLVM)](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/antwika/antwika/badges/coverage-llvm.json&style=plastic)](https://github.com/antwika/antwika/actions/workflows/ci.yml)
[![License: Apache 2.0](https://img.shields.io/badge/License-Apache%202.0-blue.svg?style=plastic)](LICENSE)

A C++23 game project built with CMake, Conan, and GoogleTest, developed inside VS Code Dev Containers for a fully reproducible toolchain across Linux (GNU/LLVM) and Windows (MinGW).

## Project structure

```
src/
├── apps/
│   ├── game/
│   ├── life/
│   ├── poker/
│   ├── sudoku/
│   └── task_worker/
└── libs/
    ├── ecs/
    ├── engine/
    ├── event/
    ├── holdem/
    ├── log/
    ├── reducer/
    ├── replay/
    ├── scheduler/
    ├── time/
    └── wfc/
blog/
```

Each library and app has its own `CMakeLists.txt`, `include/`, `src/`, and `tests/` directory.
See [`docs/STYLE_GUIDE.md`](docs/STYLE_GUIDE.md) for the project's C++/CMake/Python coding conventions.

`blog/` holds write-ups about notable changes to the project — see [`blog/001-building-a-deterministic-replay-system.md`](blog/001-building-a-deterministic-replay-system.md) for the design and requirements behind the replay system below, and [`blog/003-an-entity-component-system-with-nowhere-to-hide-a-mutation.md`](blog/003-an-entity-component-system-with-nowhere-to-hide-a-mutation.md) for the `antwika::ecs` and `antwika::reducer` libraries under `libs/ecs/` and `libs/reducer/`.

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

### Choosing a graphics backend

Builds use the `null` graphics backend, which opens windows that draw nothing and needs no display.
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
`build/bin/antwika_gfx_demo` opens a window and draws until you close it -- under the `null` backend there is nothing to close, so that build runs until interrupted.
The selection lives in the untracked `.vscode/gfx-backend`, which makes it yours rather than the repository's.

## Replays

The engine runs on a fixed timestep and every event dispatched during a run is tick-stamped, so a run can be recorded and later reloaded to reproduce the exact same resulting state:

```sh
build/bin/antwika_game --record demo.replay   # run once, save the input as a replay
build/bin/antwika_game --replay demo.replay   # reload it, reproducing the same run
```

Both modes go through the same `antwika::game::bootstrap()` entry point and the same fixed-timestep tick loop (`antwika::replay::EngineLoop`) — replay mode only differs in where each tick's events come from.
Application code (here, `apps/game`) defines its own state (`GameState`) and events (e.g. `game.score_increment`) on top of the engine's built-in per-tick event (`engine.tick`), both reacted to through the same `ITickEventSink` mechanism — see [`blog/001-building-a-deterministic-replay-system.md`](blog/001-building-a-deterministic-replay-system.md) for the full design and how to add your own.

`apps/life` is a second, independent application built on the same replay system, this time with its state held in an `antwika::ecs::World` instead of a plain struct — a Conway's Game of Life board, where each cell is an entity with a `Cell` component and a single `LifeSystem` advances every cell one generation per tick using the double-buffered `World`/`SystemScheduler` machinery described in [`blog/003-an-entity-component-system-with-nowhere-to-hide-a-mutation.md`](blog/003-an-entity-component-system-with-nowhere-to-hide-a-mutation.md):

```sh
build/bin/antwika_life --record demo.replay   # seeds a blinker, saves the input
build/bin/antwika_life --replay demo.replay   # reload it, reproducing the same run
```

Cells are toggled alive via a `life.toggle_cell` event (JSON payload `{"x":..,"y":..}`), tick-stamped exactly like `game.score_increment` — the same event-driven, replayable pattern applied to ECS state instead of a hand-rolled reducer.

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
