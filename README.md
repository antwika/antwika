[![CI](https://img.shields.io/github/actions/workflow/status/antwika/antwika/ci.yml?branch=main&style=plastic&label=CI)](https://github.com/antwika/antwika/actions/workflows/ci.yml)
[![Coverage (GNU)](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/antwika/antwika/badges/coverage-gnu.json&style=plastic)](https://github.com/antwika/antwika/actions/workflows/ci.yml)
[![Coverage (LLVM)](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/antwika/antwika/badges/coverage-llvm.json&style=plastic)](https://github.com/antwika/antwika/actions/workflows/ci.yml)
[![License: Apache 2.0](https://img.shields.io/badge/License-Apache%202.0-blue.svg?style=plastic)](LICENSE)

A C++23 game project built with CMake, Conan, and GoogleTest, developed inside VS Code Dev Containers for a fully reproducible toolchain across Linux (GNU/LLVM) and Windows (MinGW).

## Project structure

```
src/
├── apps/
│   └── game/
└── libs/
    ├── engine/
    ├── event/
    ├── log/
    ├── replay/
    └── time/
docs/
├── PLAN.md
├── CHECKLIST.md
└── notes/
```

Each library and app has its own `CMakeLists.txt`, `include/`, `src/`, and `tests/` directory.

`docs/PLAN.md` and `docs/CHECKLIST.md` describe the design and status of the
engine's replay system; `docs/notes/` holds one file per checklist item with
the rationale behind it and any issues found while building it.

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

After the build completes, run the compiled binary on your target machine:

- Linux: `build/bin/antwika_game`
- Windows: `build/bin/antwika_game.exe`

## Replays

The engine runs on a fixed timestep and every event dispatched during a run
is tick-stamped, so a run can be recorded and later reloaded to reproduce
the exact same resulting state:

```sh
build/bin/antwika_game --record demo.replay   # run once, save the input as a replay
build/bin/antwika_game --replay demo.replay   # reload it, reproducing the same run
```

Both modes go through the same `antwika::game::bootstrap()` entry point and
the same fixed-timestep tick loop (`antwika::replay::EngineLoop`) — replay
mode only differs in where each tick's events come from. Application code
(here, `apps/game`) defines its own state (`GameState`) and events (e.g.
`game.score_increment`) on top of the engine's built-in per-tick event
(`engine.tick`), both reacted to through the same `ITimedEventSink`
mechanism — see `docs/PLAN.md` for the full design and
`docs/notes/06-extendable-events.md`/`docs/notes/11-state-application-concern.md`
for how to add your own.

## Testing

Tests are built with GoogleTest and registered with CTest. From the `build` directory:

```sh
ctest --output-on-failure
```

A helper script checks for unused test doubles (mocks/fakes that no `.cpp` file includes):

```sh
python3 scripts/check_unused_test_doubles.py
```

### Coverage

The GNU and LLVM toolchains build with instrumentation via the `conan-coverage` CMake preset, which configures into its own `build-coverage/` directory (separate from `build/`) so switching between a regular and a coverage build never leaves stale, uninstrumented object files behind. Report line coverage with `gcovr`:

```sh
cmake --preset conan-coverage
cmake --build build-coverage
ctest --test-dir build-coverage
gcovr --root . --filter 'src/.*' --exclude '.*/tests/.*' --print-summary build-coverage
```

CI runs this on every push to `main` for the GNU and LLVM toolchains (not MinGW, which doesn't support `--coverage`) and publishes the resulting percentage as the badges above.

### Badges

Coverage badges report **line / function / branch** coverage. For example, **95% / 80% / 75%** indicates 95% line coverage, 80% function coverage, and 75% branch coverage.

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
