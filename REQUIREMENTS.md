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
- Running the same set of `Scheduler::schedule()`/`run()` calls twice from scratch must produce identical `run()` output both times, proven by a test rather than asserted by inspection.
- `Scheduler::run()` must dispatch ready jobs in priority order (higher priority first) with equal-priority jobs run FIFO by submission order.
- `Scheduler::run()`'s `budget` parameter must be the only mechanism controlling how many jobs run per call; no job may run outside a `run()` call.
- Graphics access must go through a backend-agnostic abstraction, and no file under `src/` may reference a concrete graphics framework such as SDL or raylib; an image format decoder such as `stb_image`, which owns no window, device or draw call, is not a graphics framework and may be linked into `antwika::gfx`.
- A graphics backend that cannot honour a request (including a window asked for with a zero width or height) must raise one specific, catchable error type, the same type for every backend.
- A headless graphics backend must exist, so tests, CI and replay verification can run with no display and no graphics framework installed.
- Rendering must be a write-only projection of application state, so a replay recorded against one backend reproduces the same state under any other.
- A window event must say which window it refers to, since a backend pumps a single event queue for every window it owns.
- A backend must declare how many windows it can hold open at once, and refuse to exceed it, rather than every backend being required to support several; raylib keeps its one window in global state and cannot.
- Polling a graphics backend for events must reach an empty queue, so a caller draining it between frames terminates.
- Text drawing must use one built-in fixed-cell bitmap font, defined by `antwika::gfx` and drawn identically by every backend, so a caller can lay text out arithmetically instead of asking a backend to measure it.
- Image assets must be decoded to pixels once, by `antwika::gfx` itself rather than by each backend, so every backend uploads byte-identical pixels; a backend receives a decoded bitmap and never a file.
- Image decoding must accept a byte stream rather than a path, so `antwika::gfx` opens no files and every decoder failure is provable headlessly.
- A texture must be created through the renderer that will draw it, and both destroying that texture after its renderer's window has closed and drawing it through another renderer must be safe.
- A blit whose source rectangle reaches outside its texture must be refused identically by every backend, since that is the one case where the underlying frameworks disagree.
- A window's close request must reach the engine only as replayable input through `IReplaySource`, never by short-circuiting the tick loop.
- UI layout must be a pure function of the described UI and the canvas size, computed arithmetically from the built-in font's metrics without asking a graphics backend to measure anything.
- UI layouts must nest, and a container must be able to take its size from the content of children it has not seen yet.
- A widget must never draw outside the container it was declared in, since the graphics abstraction offers no clipping.
- Leftover pixels from integer division in a layout must be distributed by a specified, tested rule rather than incidentally.
- No file under `src/libs/ui/` may read a clock, a pointer, a keyboard or any state outside its arguments, so the same described UI and canvas always produce the same picture.
- A UI must not open or close a container through any call a caller can forget or unbalance.
- A line must include both of its endpoints, so a line whose ends coincide draws that one pixel; callers step diagonal shapes out of lines, and a dropped endpoint leaves a gap at every corner. Which pixels between the endpoints are lit is left to the backend, since nothing reads a drawn line back.
- Any projection whose inverse decides which cell a click meant must be exact integer arithmetic, so the answer is identical across toolchains and between a recording and its replay.
- Camera state that a click is interpreted against must be simulation state, folded from replayable input, never state owned by the renderer -- otherwise a replay resolves recorded clicks against a different view and reproduces different state.
- Translating input into application meaning must happen downstream of the recorder, so a replay stores the input and regenerates what it caused rather than persisting both.
- A job with unmet dependencies (via `schedule()`'s `dependsOn`) must never be dispatched until every dependency has run; dependency cycles must be unreachable through the public API, by construction (id-ordering), not by a runtime check.
- Input access must go through a backend-agnostic abstraction, and no file under `src/` may reference a concrete input framework such as SDL or raylib.
- The input backend must be selected at build time, by the `ANTWIKA_INPUT_BACKEND` CMake variable and the matching `input_backend` Conan option, which default to the graphics choice so one flag drives both.
- A headless input backend must exist, so tests, CI and replay verification can run with no display and no input framework installed.
- Live input must reach the engine only through `IReplaySource`, so a recorded interactive session replays to the same state; no second entry point may exist.
- Input events must be persisted with symbolic key and button names, never platform scancodes, so a session recorded under one backend reproduces under another.
- Translating an input event into application meaning (a click becoming a toggled cell) must happen downstream of the replay recorder, so a replay stores the input and regenerates what it caused.
- A bad input payload, or a key or button name that no key or button goes by, must raise one specific, catchable error type, the same type for every backend.
- Polling an input backend must reach an empty queue, so a caller draining it between ticks terminates; a backend reading live state rather than a queue must latch what it has already reported.
- An input backend must declare which devices it can report at all, and must never report an event for a device it does not claim.
- Two backends over one framework must never poll that framework's event queue independently; where a framework has a single queue, exactly one place may drain it and route what it finds.

## Should have

- No RNG/PRNG in the engine, and no reserved field held aside for one later.
- The GNU coverage badge should be driven toward 100%; the LLVM branch percentage should be treated as informational only, since LLVM's `gcov` emulation can't tag compiler-generated exception-unwind branches the way GCC's can.
- A feature's diff should go through independent review passes (reuse, simplification, efficiency, altitude) before being considered done, with rationale recorded for any flagged item deliberately left alone.
- Comments should default to absent, added only when the *why* is non-obvious (a hidden constraint, a subtle invariant, a workaround, surprising behavior), except for Doxygen `@brief`/`@param`/`@return` blocks on public API surface (interfaces, classes, public methods), which document the *what* for generated reference docs and are kept regardless.
- Coverage badges (GNU and LLVM) should be generated on `main` and published to a dedicated `badges` orphan branch.
- Dev container images (base, GNU, LLVM, MinGW) should be published to `ghcr.io`, tagged with both the release version and, on the latest release, `latest`.
- An interface with only one implementation should still be kept where it lets a class be unit-tested against a mock/fake in isolation (e.g. `IEventCodec`, `IFormatter`).
- A window-driven app should pace its ticks through an injected sleeper rather than a direct sleep call, so its tests still run at full speed.
- A UI's picture should be expressible as a value (a list of drawing commands), so a whole layout can be asserted as data rather than looked at or pinned call by call against a mock.

## Could have

- Notable design decisions could be written up afterward as a post under `blog/`.
- A locally built `antwika-dev-base` image could be used in place of the `ghcr.io` one for offline or iterative Dockerfile development.

## Won't have

- The engine won't include RNG/PRNG support in its current scope.
- MinGW builds won't carry coverage instrumentation (`--coverage` isn't supported by that toolchain).
- An index over replay events (to avoid the linear scan per tick in `ReplaySource::eventsFor()`) won't be built until replays are long enough for it to matter.
- Graphics backends won't be loadable at runtime; exactly one is compiled and linked per build, selected by the `ANTWIKA_GFX_BACKEND` CMake variable and the matching `gfx_backend` Conan option.
- The graphics abstraction won't include GPU, shader or 3D APIs in its current scope; drawing is limited to clearing, filling rectangles, one-pixel lines, text in the one built-in font, and blitting a loaded texture with a source rectangle and a tint.
- `antwika::gfx` won't offer pixel read-back, render targets or screenshots, since read-back is the one thing that would let rendering feed the simulation.
- The graphics abstraction won't load fonts, or offer any font beyond the built-in fixed-cell one, since a second font implies per-backend glyph metrics that would break laying text out arithmetically; a decoded bitmap has no metrics for a backend to disagree about, which is why textures are in scope and fonts are not.
- `antwika::gfx` won't report keyboard or pointer input; that travels through `antwika::input`, which does not depend on `antwika::gfx`, so reading input never requires opening a window.
- Input backends won't be loadable at runtime, for the same reasons graphics backends aren't.
- The input abstraction won't cover text or IME input, cursor capture or warping, gamepads, or touch, and won't say which window an event arrived at, since every application here has one window.
- `antwika::input` won't fold input events into held device state or bind them to named actions in its current scope; an application that needs "is this button down" derives it from the edges it already receives.
- The raylib input backend won't report a keyboard in its current scope, and says so through its capabilities rather than claiming a device whose events never arrive.
- Walkers in `apps/game` won't collide: two may occupy one cell, because nothing requires otherwise and a rule to avoid it would be a requirement nobody asked for.
- `Scheduler` won't include priority aging or anti-starvation: a continuous stream of higher-priority jobs can, by design, keep a lower-priority job pending indefinitely, since unconditional priority respect is the requirement, not a bug to work around.
- `antwika::ui` won't read a pointer or a keyboard, and no UI interaction will reach the engine or a replay, in its current scope; a button is told how it should look rather than working it out.
- `antwika::ui` won't clip, scroll, or draw out of declaration order, since `antwika::gfx` offers no scissor and no z-order; a container that cannot fit its content shrinks it in proportion instead.
- `antwika::ui` won't wrap text across lines, offer a main-axis alignment mode (a growing spacer expresses leading, trailing and centred content), weight how growing children share leftover room, or animate anything.
- `antwika::ui` won't carry a style stack or cascade; one plain `Theme` value is passed to a frame and nothing overrides it per widget.
