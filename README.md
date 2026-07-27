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
    └── time/
```

Each library and app has its own `CMakeLists.txt`, `include/`, `src/`, and `tests/` directory.

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
