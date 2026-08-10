# antwika

A C++ project built around deterministic simulation: the libraries under `src/libs/` and the applications under `src/apps/` share one engine, one replay format and one rendering seam, with the graphics, sound and network backends chosen at build time.

## Building

The build needs a Conan profile named in `CONAN_PROFILE`, matching a file under `profiles/host/`.

```sh
export CONAN_PROFILE=gcc-linux-x86_64
scripts/build.sh
```

`scripts/build.sh` reads the backend selection from `.vscode/gfx-backend`, `.vscode/sound-backend` and `.vscode/network-backend`, installs the matching dependencies, configures CMake and builds into `build/`.
Use `scripts/select_backend.sh` to change a selection.

`cmake --install build --prefix <somewhere>` gives each application a directory of its own under `bin/`, holding the executable, its assets and its notices.
The directory is the unit that has to stay together, because `antwika::app::assetPath()` resolves a name against the running executable's own directory rather than the working directory.

## Tests

`scripts/build.sh` runs the suites itself once the build succeeds.

They are split because the sound and network conformance suites run with no display, and everything else runs under Xvfb when the graphics backend is not `null`.

```sh
ctest --test-dir build \
    -R 'SoundBackendConformance|NetworkBackendConformance'

xvfb-run -a ctest --test-dir build \
    -E 'SoundBackendConformance|NetworkBackendConformance'
```

Running a bare `ctest` instead will fail the sound suites, which have no audio device to open.

## Drawing without a window

Every graphical app has a draw test that builds a scene and paints one frame through `antwika::gfx::mocks::MockRenderer`, asserting the calls the scene makes rather than the pixels they would produce.

```sh
ctest --test-dir build -R DrawTest
```

No test rasterises a frame, so none of them needs a display or a graphics backend.

## Coverage

Coverage is gated at 100% of lines, functions and branches.

```sh
cmake --preset conan-coverage
cmake --build build-coverage -j"$(nproc)"
scripts/coverage.sh --build-dir build-coverage --summary coverage-summary.json
python3 scripts/check_full_coverage.py --summary coverage-summary.json
```

Run the tests against `build-coverage` first, with the same split as above.
Clear any stale `.gcda` files before trusting a second run.

## Bundled assets

`assets/fonts/RobotoMono-Regular.ttf` is Roboto Mono 3.001, taken verbatim from [`googlefonts/RobotoMono`](https://github.com/googlefonts/RobotoMono).

It is licensed under the SIL Open Font License 1.1, and `assets/fonts/LICENSE.txt` is that repository's `OFL.txt` copied byte for byte.
Copying a licence under a different file name is allowed; editing one is not, and this one is not edited.

`antwika_embed_binary()` in `cmake/AntwikaEmbedBinary.cmake` turns the file into a C++ source of bytes at configure time, which is how `antwika::gfx` draws text without opening a file.

## Licence

antwika's own code is under the Apache License 2.0.
`LICENSE` holds that licence unedited, and `NOTICE` holds the copyright.

`assets/` is not covered by it: the font described above stays under the SIL Open Font License 1.1, whatever the code around it is licensed under.

`THIRD_PARTY.txt` records what a built executable carries besides antwika's own code, which depends on the backends the build selects.
`antwika_bundle_app()` copies it, `LICENSE`, `NOTICE` and the font's licence next to every application, in the build tree and in an install alike, so an executable never travels without them.
A library-only install finds the same four under `share/doc/antwika`.

That list names the third-party components without reproducing their licence texts, which is enough while only source is published.
A release that ships binaries needs more, and `THIRD_PARTY.txt` ends with what.

## Style

`docs/STYLE_GUIDE.md` gives the comment and documentation rules, and `scripts/check_comment_style.py` enforces the checkable ones.
