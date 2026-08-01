# Getting started

## Dev containers

The project is developed inside VS Code Dev Containers so the toolchain is reproducible.
Open the repository in VS Code and run `Dev Containers: Reopen in Container`, then pick **GNU**, **LLVM** or **MinGW**.
The **Base** container is only the shared foundation the other three build on and is not meant for building the project.

Switching between containers may require removing the `build` directory first.

## Build and test

`Ctrl+Shift+B` runs the default build task, which is the same sequence as this:

```sh
conan install . -of build \
  -pr:b=./profiles/build/${CONAN_PROFILE} \
  -pr:h=./profiles/host/${CONAN_PROFILE} \
  --build=missing -s build_type=Release --lockfile=conan.lock

cmake --preset conan-release
cmake --build build -j24
ctest --test-dir build --output-on-failure
```

`CONAN_PROFILE` names a profile that exists under both `profiles/build/` and `profiles/host/`, for example `gcc-linux-x86_64`, `clang-linux-x86_64` or `mingw-windows-x86_64`.

`CMakePresets.json` includes `build/CMakePresets.json` unconditionally, so CMake refuses to read *any* preset until that first `conan install . -of build` has run.
On a fresh clone, run it before anything else.

To iterate on one module, build and run only its test binary:

```sh
cmake --build build --target antwika_replay_tests -j24
ctest --test-dir build -R antwika_replay_tests --output-on-failure
build/bin/antwika_replay/antwika_replay_tests \
    --gtest_filter='ReplayReaderTest.*'
```

## Choosing a graphics and input backend

Builds default to the `null` graphics and input backends, which need no framework, draw nothing, report no input, and need no display.
The real backends are `sdl3` and `raylib`.

From VS Code, `Tasks: Run Task > Select gfx backend` makes the choice once and `Ctrl+Shift+B` honours it from then on; `Select sound backend` is its counterpart for sound.
From a terminal:

```sh
scripts/select_backend.sh gfx sdl3     # and/or: select_backend.sh sound sdl3
scripts/build.sh
```

Either way the build lands in `build/`, whatever is selected.
The selections live in the untracked `.vscode/gfx-backend` and `.vscode/sound-backend`.

Or by hand:

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

By hand, a real backend builds into its own directory (`build-sdl3/`, `build-raylib/`), which is what CI does and why the backend has to appear in the preset name.
`scripts/build.sh` deliberately does not: it keeps one `build/`, since a developer switches between backends one at a time where CI's legs run in parallel.
Each configuration has its own lockfile, because selecting a backend changes the dependency graph.

The Conan `gfx_backend` option sets the `ANTWIKA_GFX_BACKEND` CMake variable, which names a directory under `backends/`; an unknown value fails at configure time with the list of ones that exist.
Input is selected the same way, by `-o input_backend=` and `ANTWIKA_INPUT_BACKEND`, defaulting to `auto` — which resolves to whatever `gfx_backend` is, so one flag usually drives both.
Setting them apart is allowed for input with no window (`-o gfx_backend=null -o input_backend=sdl3`) or a window with no input.
Sound is selected the same way again, by `-o sound_backend=` and `ANTWIKA_SOUND_BACKEND`, with values `null` and `sdl3` — but it defaults to `null` rather than to `auto`.
Input follows graphics because a window nobody can click is useless; sound is orthogonal, and following would mean every existing `sdl3` build silently began opening an audio device.
`raylib` is absent from its values because it does not implement that seam, so Conan refuses the value before downloading anything.

```sh
conan install . -of build-sdl3 -o gfx_backend=sdl3 -o sound_backend=sdl3 ...
```

A lockfile is per *framework* rather than per subsystem, so a `sound_backend=sdl3` build uses `conan-sdl3.lock` like any other sdl3 build.

Naming two *different* real frameworks anywhere is refused, in `conanfile.py` and again in `backends/CMakeLists.txt`.
Graphics and input would fight over one process-global event queue, and a second framework of any kind doubles the dependency graph of a build that only needs one.
A selection naming a directory that implements no such subsystem is refused at configure time too, rather than failing much later at link.

Without a display, set `SDL_VIDEODRIVER=dummy` for the SDL build, or wrap any backend in `xvfb-run`.
`SDL_AUDIO_DRIVER=dummy` is the sound equivalent, and sound needs no display at all.

## Running the apps

```sh
build/bin/antwika_game/antwika_game                        # empty grid, runs until quit
build/bin/antwika_game/antwika_game --record demo.replay   # or --replay demo.replay
build/bin/antwika_life/antwika_life
build/bin/antwika_life/antwika_life --record demo.replay
build/bin/antwika_task_worker/antwika_task_worker --record demo.replay
build/bin/antwika_poker/antwika_poker --record demo.replay
build/bin/antwika_sudoku/antwika_sudoku [--puzzle my-puzzle.txt]
build/bin/antwika_tower_defence/antwika_tower_defence               # or --record / --replay
build/bin/antwika_gfx_demo/antwika_gfx_demo
build/bin/antwika_gfx3d_demo/antwika_gfx3d_demo                  # spinning cube, a fixed frame count
build/bin/antwika_sound_demo/antwika_sound_demo [--file my-sound.wav]   # silent under null
```

On Windows the binaries carry a `.exe` suffix.

Every tick-loop app takes `--record <path>` and `--replay <path>`, parsed by `antwika::replay`'s `ReplayCli`.
`antwika_poker` additionally takes `--tick-delay-ms <n>`, and `antwika_sudoku` takes `--puzzle <path>`.
A `--record` run only writes its file once the run ends, so a run you interrupt with `Ctrl+C` saves nothing.
Sample sessions to pass to `--replay` are checked in under `src/apps/*/replays/`.

See each app's page for what it does and which libraries it composes: [game](apps/game.md), [life](apps/life.md), [task_worker](apps/task_worker.md), [poker](apps/poker.md), [tower_defence](apps/tower_defence.md), [sudoku](apps/sudoku.md), [gfx_demo](apps/gfx_demo.md), [gfx3d_demo](apps/gfx3d_demo.md), [sound_demo](apps/sound_demo.md).

## Coverage build

Coverage uses a separate build directory and works on GNU and LLVM only, not MinGW.

```sh
cmake --preset conan-coverage
cmake --build build-coverage -j24
ctest --test-dir build-coverage -j"$(nproc)"
gcovr --root . --filter 'src/.*' --exclude '.*/tests/.*' --print-summary build-coverage
```

See [Contributing](Contributing.md) for what CI does with the result.

## Updating lockfiles

After a dependency bump in `conanfile.py`:

```sh
scripts/update_lockfiles.sh   # or: Tasks: Run Task > Update Conan lockfiles
```

It re-resolves every lockfile from scratch against every profile under `profiles/host/`, since CI builds all of them against the same files.
