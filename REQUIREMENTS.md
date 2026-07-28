# Requirements

Requirements for the Antwika project, gathered from `README.md`, `blog/001-building-a-deterministic-replay-system.md`, the build/CI configuration, and the checker scripts under `scripts/`, phrased as MoSCoW statements.

## Must have

- The engine and game must be written in C++23, built with CMake (>= 3.28) and Conan, and tested with GoogleTest registered through CTest.
- The project must build across three toolchains: GNU, LLVM, and MinGW (Windows, via cross-compilation), each inside its own reproducible VS Code Dev Container.
- Compiler warnings (`-Wall -Wextra -Wpedantic -Wsuggest-override`) must be treated as errors by default.
- The compiled game binary must run standalone after a build (`build/bin/antwika_game` or `.exe`).
- Each library and app must own its own `CMakeLists.txt`, `include/`, `src/`, and `tests/` directory.
- Events occurring during a run must be serializable to, and deserializable from, a replay.
- Loading a replay must deterministically reproduce the same state a live run reached, proven by a test rather than asserted by inspection.
- Everything that happens during engine execution must be replayable; no event category is exempt.
- The engine must run on a fixed timestep (discrete ticks), not wall-clock time.
- `Event` must be extendable so application code can define its own event kinds and still benefit from the engine's built-in ones, through one uniform mechanism with no special-casing between "built-in" and "custom".
- State representation must remain an application concern; the engine core must stay domain-agnostic.
- A replay must persist only external input, not events the engine regenerates deterministically on its own (e.g. the built-in per-tick event).
- Bad replay input (wrong magic bytes, unsupported format version, truncated stream) must raise one specific, catchable error type, not a vague exception or silent misbehavior.
- Live mode and replay mode must run through the same tick-loop code path, differing only in where each tick's events come from.
- The game binary must support `--record <path>` and `--replay <path>` CLI flags.
- Design must follow SOLID principles with small interfaces, and tests must be written alongside each piece of behavior rather than bolted on afterward.
- Adding a new concern to an existing, tested class must prefer composition (e.g. a decorator) over modifying that class.
- Tests must be written with GoogleTest and registered with CTest.
- Every mock/fake header under a `tests/{mocks,fakes}/include` directory must be `#include`d by at least one `.cpp` file.
- CI must enforce coverage instrumentation and reporting for the GNU and LLVM toolchains.
- Lines excluded from coverage via `GCOVR_EXCL_LINE` must carry a comment explaining why, and only after real, testable gaps have been covered by actual tests, following the procedure in `docs/confirming-unreachable-branches.md`.
- Source lines (`src/**/*.cpp`, `src/**/*.hpp`) and script lines (`scripts/*.py`, `scripts/tests/*.py`) must not exceed 80 characters.
- Comments and markdown prose must hold exactly one sentence per line, with no sentence wrapped across multiple lines.
- CI must run on every push to any branch (except `badges`) and on `v*` tags, building and testing all three toolchains.
- CI must verify that the expected binaries exist after the build for each toolchain.
- Releases must be cut via `semantic-release`, driven by Conventional Commits, publishing to `CHANGELOG.md` and GitHub releases.
- The project must be licensed under the Apache License 2.0.

## Should have

- No RNG/PRNG in the engine, and no reserved field held aside for one later.
- Live/interactive input capture (as opposed to a hand-authored input script) should stay out of scope until the engine gains a live input source.
- The GNU coverage badge should be driven toward 100%; the LLVM branch percentage should be treated as informational only, since LLVM's `gcov` emulation can't tag compiler-generated exception-unwind branches the way GCC's can.
- A feature's diff should go through independent review passes (reuse, simplification, efficiency, altitude) before being considered done, with rationale recorded for any flagged item deliberately left alone.
- Comments should default to absent, added only when the *why* is non-obvious (a hidden constraint, a subtle invariant, a workaround, surprising behavior), except for Doxygen `@brief`/`@param`/`@return` blocks on public API surface (interfaces, classes, public methods), which document the *what* for generated reference docs and are kept regardless.
- Coverage badges (GNU and LLVM) should be generated on `main` and published to a dedicated `badges` orphan branch.
- Dev container images (base, GNU, LLVM, MinGW) should be published to `ghcr.io`, tagged with both the release version and, on the latest release, `latest`.
- An interface with only one implementation should still be kept where it lets a class be unit-tested against a mock/fake in isolation (e.g. `IEventCodec`, `IFormatter`).

## Could have

- Notable design decisions could be written up afterward as a post under `blog/`.
- A locally built `antwika-dev-base` image could be used in place of the `ghcr.io` one for offline or iterative Dockerfile development.

## Won't have

- The engine won't include RNG/PRNG support in its current scope.
- The engine won't support capturing live/interactive input into a replay in its current scope; the current replay input is a hand-authored script.
- MinGW builds won't carry coverage instrumentation (`--coverage` isn't supported by that toolchain).
- An index over replay events (to avoid the linear scan per tick in `ReplaySource::eventsFor()`) won't be built until replays are long enough for it to matter.
