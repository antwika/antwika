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
│   └── task_worker/
└── libs/
    ├── ecs/
    ├── engine/
    ├── event/
    ├── log/
    ├── reducer/
    ├── replay/
    ├── scheduler/
    └── time/
blog/
```

Each library and app has its own `CMakeLists.txt`, `include/`, `src/`, and `tests/` directory.

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

- Linux: `build/bin/antwika_game`, `build/bin/antwika_life`, `build/bin/antwika_task_worker`
- Windows: `build/bin/antwika_game.exe`, `build/bin/antwika_life.exe`, `build/bin/antwika_task_worker.exe`

## Replays

The engine runs on a fixed timestep and every event dispatched during a run is tick-stamped, so a run can be recorded and later reloaded to reproduce the exact same resulting state:

```sh
build/bin/antwika_game --record demo.replay   # run once, save the input as a replay
build/bin/antwika_game --replay demo.replay   # reload it, reproducing the same run
```

Both modes go through the same `antwika::game::bootstrap()` entry point and the same fixed-timestep tick loop (`antwika::replay::EngineLoop`) — replay mode only differs in where each tick's events come from.
Application code (here, `apps/game`) defines its own state (`GameState`) and events (e.g. `game.score_increment`) on top of the engine's built-in per-tick event (`engine.tick`), both reacted to through the same `ITimedEventSink` mechanism — see [`blog/001-building-a-deterministic-replay-system.md`](blog/001-building-a-deterministic-replay-system.md) for the full design and how to add your own.

`apps/life` is a second, independent application built on the same replay system, this time with its state held in an `antwika::ecs::World` instead of a plain struct — a Conway's Game of Life board, where each cell is an entity with a `Cell` component and a single `LifeSystem` advances every cell one generation per tick using the double-buffered `World`/`SystemScheduler` machinery described in [`blog/003-an-entity-component-system-with-nowhere-to-hide-a-mutation.md`](blog/003-an-entity-component-system-with-nowhere-to-hide-a-mutation.md):

```sh
build/bin/antwika_life --record demo.replay   # seeds a blinker, saves the input
build/bin/antwika_life --replay demo.replay   # reload it, reproducing the same run
```

Cells are toggled alive via a `life.toggle_cell` event (payload `"x,y"`), tick-stamped exactly like `game.score_increment` — the same event-driven, replayable pattern applied to ECS state instead of a hand-rolled reducer.

`apps/task_worker` is a third application, this time combining `antwika::ecs` with a new `antwika::scheduler` library: a fixed pool of `Worker` entities pulls tasks off a deterministic, priority-ordered, budget-bounded `antwika::scheduler::Scheduler`, submitted over time via a `task.submit` event and, optionally, chained to an earlier task with a dependency edge:

```sh
build/bin/antwika_task_worker --record demo.replay   # submits a mixed-priority task burst
build/bin/antwika_task_worker --replay demo.replay   # reload it, reproducing the same run
```

Tasks are submitted via a `task.submit` event (payload `"id,priority,durationTicks,label[,dependsOnId]"`), tick-stamped exactly like `life.toggle_cell` — a `TaskSubmissionSink` schedules each parsed task onto the `Scheduler`, and a `TaskDispatchSystem` runs the scheduler each tick with that tick's idle-worker count as its budget, so no more tasks start than there are free workers.
See [`blog/005-a-job-scheduler-and-a-worker-pool-that-cant-lie-to-itself.md`](blog/005-a-job-scheduler-and-a-worker-pool-that-cant-lie-to-itself.md) for the full design.

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
cmake --build build-coverage
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
