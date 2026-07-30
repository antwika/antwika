# Plan: adding a `gfx` library

A plan for adding graphics support to Antwika: an abstraction over opening,
closing and rendering to one or more windows, with the concrete graphics
framework (SDL, raylib, ...) chosen at build time and never named by
Antwika's own source.

## Goals

- One abstraction that Antwika code renders through, expressed only in terms
  of windows, surfaces and primitives.
- No file under `src/` may `#include` an SDL, raylib or other framework
  header, or link against one.
- Swapping the framework is a build configuration change, not a code change.
- The default build depends on nothing new, so CI keeps building and passing
  with no graphics framework installed.
- Rendering stays outside the deterministic tick path, so replays remain
  reproducible.

## Decision: static backends, selected at build time

Backends are ordinary static libraries.
Exactly one is compiled and linked into a given build, selected by a CMake
cache variable.
There is no runtime loading, no `dlopen`, and no C ABI boundary.

This was chosen over runtime plugin loading deliberately:

- A C++ ABI cannot be relied on across a `dlopen` boundary between the GNU,
  LLVM and MinGW toolchains this project builds, so runtime loading would
  have forced a hand-written C ABI plus a translation shim on both sides.
- Exceptions cannot cross a C ABI boundary, so `GfxError` would have become
  return codes plus a `lastError()` call, converted back into exceptions by
  an adapter.
- Runtime loading brings ABI versioning, symbol resolution and library
  lifetime failure modes that are hard to reach from a test, which is
  expensive under a 100% coverage gate.

None of that cost buys anything the stated goal needs.
Static selection meets every goal above, and the `IGfxBackend` seam is
identical either way, so runtime loading remains addable later behind the
same interface if a real need for it appears.

## Module layout

```
src/libs/gfx/        antwika::gfx           interfaces, value types,
                                            GfxError, NullBackend
backends/null/       antwika::gfx_backend   selects NullBackend
backends/sdl3/       antwika::gfx_backend   opt-in, requires SDL3
backends/raylib/     antwika::gfx_backend   opt-in, requires raylib
src/apps/gfx_demo/   antwika_gfx_demo       end-to-end composition
```

`backends/` sits at the repository root, deliberately outside `src/`.
That placement is load-bearing and is explained under
[Coverage](#coverage-the-main-constraint).

Every backend directory builds a target named `antwika_gfx_backend`, aliased
to `antwika::gfx_backend`.
Only one is ever added to the build, so the fixed name never collides, and
apps link `antwika::gfx_backend` without knowing which backend they got.

## `antwika::gfx`

Contents of `src/libs/gfx/include/antwika/gfx/`:

| Header | Purpose |
| --- | --- |
| `Color.hpp`, `Point.hpp`, `Size.hpp`, `Rect.hpp` | plain value types |
| `WindowDesc.hpp` | title, size, and creation flags for a new window |
| `WindowEvent.hpp` | close requested, resized, key, pointer |
| `IWindow.hpp` | one window's lifetime and properties |
| `IRenderer.hpp` | drawing operations against one window |
| `Glyphs.hpp`, `TextLayout.hpp` | the one built-in fixed-cell font, and measuring it |
| `IGfxBackend.hpp` | window factory and event pump |
| `SelectedBackend.hpp` | declares the build-time-selected factory function |
| `NullBackend.hpp` | headless backend that draws nothing |
| `GfxError.hpp` | the one exception type for this failure category |

The library depends only on `antwika::log`.
It deliberately does **not** depend on `antwika::event`; see
[Keeping rendering out of the tick path](#keeping-rendering-out-of-the-tick-path).

### Interface sketch

```cpp
class IWindow
{
public:
    virtual ~IWindow() = default;

    [[nodiscard]] virtual bool isOpen() const = 0;
    [[nodiscard]] virtual Size size() const = 0;
    [[nodiscard]] virtual IRenderer &renderer() = 0;

    virtual void setTitle(std::string_view title) = 0;
    virtual void close() = 0;
};

class IRenderer
{
public:
    virtual ~IRenderer() = default;

    virtual void clear(Color color) = 0;
    virtual void drawRect(Rect rect, Color color) = 0;
    virtual void drawText(
        Point origin,
        std::string_view text,
        std::uint32_t scale,
        Color color) = 0;
    virtual void present() = 0;
};

class IGfxBackend
{
public:
    virtual ~IGfxBackend() = default;

    [[nodiscard]] virtual std::string_view name() const = 0;

    /**
     * @throws GfxError If the window could not be created.
     */
    [[nodiscard]] virtual std::unique_ptr<IWindow> createWindow(
        const WindowDesc &desc) = 0;

    /**
     * @brief Take the next pending event, if any.
     * @return The event, or nullopt when the queue is empty.
     */
    [[nodiscard]] virtual std::optional<WindowEvent> pollEvent() = 0;
};
```

Multiple windows fall out of `createWindow()` returning an independent
`IWindow` per call; nothing in the interface assumes a single window.

### The selection seam

`antwika::gfx` **declares** one function and never defines it:

```cpp
// src/libs/gfx/include/antwika/gfx/SelectedBackend.hpp
namespace antwika::gfx
{
    /**
     * @brief Create the graphics backend chosen at build time.
     *
     * Declared here but defined by whichever backend under backends/ was
     * selected via ANTWIKA_GFX_BACKEND, so no code under src/ names a
     * concrete graphics framework.
     *
     * @return The selected backend, never null.
     * @throws GfxError If the underlying framework failed to initialise.
     */
    [[nodiscard]] std::unique_ptr<IGfxBackend> makeSelectedBackend();
} // namespace antwika::gfx
```

Each backend supplies the definition, for example
`backends/sdl3/src/SelectedBackend.cpp`.
A declaration creates no link-time dependency, so `antwika::gfx` stays a leaf
and `antwika::gfx_backend` depends on it, not the other way round.

An app's `main.cpp` then reads:

```cpp
auto backend = antwika::gfx::makeSelectedBackend();
auto window = backend->createWindow({"Antwika", 800, 600});
```

and is injected into the rest of the app as `IGfxBackend &`, matching how
`apps/life`'s `main.cpp` already constructs collaborators and hands them to
`bootstrap()`.

## Build-time selection

Root `CMakeLists.txt`:

```cmake
set(ANTWIKA_GFX_BACKEND "null" CACHE STRING
    "Graphics backend to compile and link (null, sdl3, raylib)")

set_property(CACHE ANTWIKA_GFX_BACKEND PROPERTY STRINGS null sdl3 raylib)

add_subdirectory(backends)
```

`backends/CMakeLists.txt`:

```cmake
if(NOT IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${ANTWIKA_GFX_BACKEND}")
    message(FATAL_ERROR
        "Unknown ANTWIKA_GFX_BACKEND '${ANTWIKA_GFX_BACKEND}'")
endif()

add_subdirectory(${ANTWIKA_GFX_BACKEND})
```

A wrong value fails at configure time with a clear message rather than at
link time with an unresolved `makeSelectedBackend`.

### Conan

The framework packages are pulled in only when selected, via a Conan option
on the root `conanfile.py`:

```python
options = {"gfx_backend": ["null", "sdl3", "raylib"]}
default_options = {"gfx_backend": "null"}

def requirements(self):
    self.requires("nlohmann_json/3.12.0", override=True)
    self.requires("json-schema-validator/2.4.0")

    if self.options.gfx_backend == "sdl3":
        self.requires("sdl/3.2.6")
    elif self.options.gfx_backend == "raylib":
        self.requires("raylib/5.5")

def generate(self):
    tc = CMakeToolchain(self)
    tc.cache_variables["ANTWIKA_GFX_BACKEND"] = str(self.options.gfx_backend)
    tc.generate()
    CMakeDeps(self).generate()
```

Adding an explicit `generate()` replaces the current `generators` tuple, so
one flag drives both Conan and CMake:

```sh
conan install . -of build -o gfx_backend=sdl3 ...
```

Two consequences to plan for:

- `conan.lock` is generated per configuration; the non-default backends need
  their own lockfiles, or the pinned lockfile must be regenerated when a
  backend is selected.
- The framework name appears in `conanfile.py` as a build option value.
  That is build configuration, not a source dependency, and no `src/` file
  is affected.
  This is accepted rather than worked around: a per-backend
  `backends/sdl3/conanfile.py` would keep the name out of the root file, but
  costs a second `conan install` for every backend developer, which is a
  poor trade for one build option value.

## Coverage: the main constraint

CI enforces 100% line, function and branch coverage on the GNU leg, over
`--filter 'src/.*'`.

Backend code cannot meet that bar: CI has no display, no SDL and no raylib,
so not one line of `backends/sdl3/` would ever execute.
Putting backends under `src/libs/` would therefore make the coverage gate
unsatisfiable.

Placing `backends/` outside `src/` puts it outside the gcovr filter, so:

- `antwika::gfx` — interfaces, value types, `NullBackend`, `GfxError` — lives
  under `src/` and is held to 100% coverage like every other library.
- Backend implementations live under `backends/`, are excluded from the
  coverage gate by construction, and are verified by the conformance suite
  instead.

No new `GCOVR_EXCL_LINE` is needed anywhere.

The style checkers glob `src/**` and would silently skip `backends/`, so
`scripts/check_line_length.py` and
`scripts/check_one_sentence_per_line.py` should have `backends/**/*.cpp` and
`backends/**/*.hpp` added to their `CPP_GLOBS`, with their own script tests
updated to match.
Style still applies to backend code; only the coverage gate does not.

## Toolchain and CI integration

Three pieces of existing plumbing need extending, none of them optional.

**The CI executable check.** `.github/workflows/build.yml` verifies a
hardcoded list of expected binaries after every build, with a shorter
apps-only list on MinGW. `antwika_gfx_tests` and `antwika_gfx_demo` must be
added to the main list, and `antwika_gfx_demo` to the MinGW one.

**MinGW runtime DLLs.** Each app copies `libgcc_s_seh-1.dll`,
`libstdc++-6.dll` and `libwinpthread-1.dll` next to its binary in a
`POST_BUILD` step, and `apps/gfx_demo` needs the same block. A framework
backend adds its own runtime DLL (`SDL3.dll`, ...) to that copy list, which
is backend-specific and so belongs in `backends/<name>/CMakeLists.txt`
rather than in the app.

**Install rules.** Every module mirrors the same `ARCHIVE`/`LIBRARY`/
`RUNTIME` destinations, header directory, and `antwika::`-namespaced export
set, and `antwika::gfx` follows that pattern unchanged. Whether backends
install at all is a genuine question: exporting `antwika_gfx_backend` from
an install tree pins consumers to whichever backend that tree was built
with. The recommendation is to install them anyway and document that an
installed Antwika is backend-specific.

**CI stays on the null backend**, because nothing else is possible without a
display or the framework packages. The cost is that backend code can bitrot
silently between the phases that touch it. Once Phase 4 lands, a cheap
mitigation is a configure-and-build-only job with `-o gfx_backend=sdl3` that
never runs the binary — it catches compile breakage without needing a
display.

## Verifying backends: a conformance suite

Because backends are untested by the coverage gate, they need a shared,
reusable test suite rather than per-backend ad-hoc tests.

`src/libs/gfx/tests/conformance/` builds an INTERFACE target
`antwika::gfx::tests::conformance` exporting a GoogleTest fixture
parameterised over a backend factory.
It asserts the behaviour every backend must share, for example:

- `createWindow()` returns a window that reports `isOpen()`.
- `close()` makes `isOpen()` false and is idempotent.
- Two `createWindow()` calls yield independent windows.
- `pollEvent()` returns `nullopt` on a quiet queue rather than blocking.
- A failed window creation throws `GfxError`, not a bare exception.

CI instantiates the suite against `NullBackend`.
`backends/sdl3/tests/` and `backends/raylib/tests/` instantiate the exact
same suite, run locally or on a display-capable runner.
A backend is "done" when it passes the suite unmodified — which is also the
strongest available check that the abstraction is not quietly shaped around
one framework.

## Keeping rendering out of the tick path

Requirement: a replay must reproduce state deterministically, and everything
during engine execution must be replayable.
Rendering must not compromise that.

- `antwika::gfx` does not depend on `antwika::event`.
  Translating a `gfx::WindowEvent` into an `antwika::event::Event` is an
  adapter that belongs in the app, so window and input handling enter the
  engine only through the existing `IReplaySource` path like any other
  external input.
- Rendering is a read-only projection of state.
  For an ECS app it is an `ecs::ISystem` in a present phase, exactly like the
  existing `PrintSystem` in `apps/life`.
  A render system reads components and never writes them.
- The check that this holds: record a replay under a real backend, replay it
  under `NullBackend`, and assert the final state is identical.
  That is the same style of proof used for replay determinism today, and it
  belongs in the phase that introduces the first real backend.

`NullBackend` is therefore not a placeholder; it is what lets tests, CI and
headless replay verification run with no display present.

## Phases

Each phase ends with a green build and test run, and is done in its own git
worktree.

### Phase 1 — `antwika::gfx`

`src/libs/gfx/` with the value types, interfaces, `GfxError`, `NullBackend`
and `MockGfxBackend`/`MockWindow`/`MockRenderer` under `tests/mocks/`.
Full unit tests, 100% coverage, no new dependency.
Nothing else in the repository changes yet.

### Phase 2 — selection machinery and a demo

`ANTWIKA_GFX_BACKEND`, `backends/CMakeLists.txt`, `backends/null/`, and
`src/apps/gfx_demo/` opening a window through `makeSelectedBackend()` and
drawing a few rectangles.
Under the default `null` backend the demo runs headless in CI and proves the
composition, before any framework exists.

### Phase 3 — conformance suite

The shared fixture, instantiated against `NullBackend`.
This locks the contract down before a real backend can bend it.

### Phase 4 — `backends/sdl3`

The first real backend, plus the Conan option and its lockfile.
SDL3 is the recommended first target: its C API maps almost directly onto
`IGfxBackend`, so the backend stays thin.
Ends with the replay-equivalence test from
[Keeping rendering out of the tick path](#keeping-rendering-out-of-the-tick-path).

### Phase 5 — `backends/raylib`

The second backend is where the abstraction is actually proven.
An interface validated against one framework is that framework with extra
steps; expect Phase 5 to force small changes to `IGfxBackend`, and treat the
interface as unstable until it lands.

### Phase 6 — a render system for `apps/life`

Replace or complement `PrintSystem` with a gfx-backed render system, giving
the project a visible, replayable, deterministic demo.
Then the blog post, per the project's usual practice of writing up a design
after the fact.

### Phase 7 — `apps/poker` draws itself

Done ahead of Phase 6, and for the same reasons: poker is the project's
showcase, and a hand history is exactly the kind of thing worth seeing
rather than reading.

It added `IRenderer::drawText` (see
[Deferred deliberately](#deferred-deliberately)) and shaped how an app hangs
rendering off the tick loop:

- `poker::snapshotOf()` produces an immutable `poker::TableSnapshot`, the
  spectator's counterpart to `holdem::TableView`.
  `poker::TableScene` draws only that, so it structurally cannot reach the
  `Table` -- write-only in the type system rather than by promise, which is
  also what keeps its tests to struct literals against a mock renderer.
- `poker::TableRenderSink` is an ordinary `ITickEventSink`, registered after
  the sink that steps the table.
  That ordering is positional and nothing but a test enforces it.
- `poker::WindowCloseSource` decorates the `IReplaySource` and appends
  `engine.stop` once the window has gone.
  Pumping the queue there rather than beside the drawing is what makes the
  close cost no latency: the loop asks a source for a tick's events *before*
  stepping, so a close seen now stops the session now.
- Pacing needed `antwika::time::ISleeper`, since a 250-tick session is
  otherwise over in milliseconds, and a bare `sleep_for` would have made the
  bootstrap tests spend real seconds each.

One trap worth recording: holding the final frame open until the window
closes hangs under any backend that never reports a close, which is exactly
what `NullBackend` does.
It is therefore gated on the app having been asked to pace itself at all.
The equivalence proof from
[Keeping rendering out of the tick path](#keeping-rendering-out-of-the-tick-path)
is in place both as a unit test and by hand: a session recorded under sdl3
replays under `null` to identical output.

## Documentation and requirements to update

- `REQUIREMENTS.md` gains Must-have entries: graphics access must go through
  a backend-agnostic abstraction; no file under `src/` may reference a
  concrete graphics framework; the backend must be selected at build time;
  bad graphics operations must raise one specific catchable error type.
- `REQUIREMENTS.md` gains a Won't-have entry: no GPU, shader or 3D API in
  this scope, and no runtime-loadable backends.
- `CLAUDE.md` and `README.md` gain the `ANTWIKA_GFX_BACKEND` /
  `-o gfx_backend=` build instructions.
- `docs/STYLE_GUIDE.md` notes that `backends/` follows the same style rules
  but is outside the coverage gate.

## Deferred deliberately

- **Textures and sprites.** These need resource-handle lifetime rules that
  are worth designing against a real use case rather than guessing at now.
  Phase 1 was limited to clear, rectangle and present.

  Text arrived later, in Phase 7, once a real use case asked for it: a poker
  table with no card ranks, names or chip counts on it is not worth looking
  at.
  It is deliberately *not* a texture or a font resource.
  `antwika::gfx` owns one fixed-cell 5x7 bitmap font as data, every backend
  paints it out of `drawRect`-equivalent calls, and `textSize()` measures it
  arithmetically.
  Nothing is loaded, so there is still no resource-handle lifetime question,
  and no new dependency: SDL_ttf was considered and rejected on those
  grounds.
  The fixed cell is the load-bearing part -- it is what lets a scene lay text
  out without asking a backend anything, and what stops two backends drawing
  different pictures.
  It is also why `backends/raylib` does not use raylib's own `DrawText`,
  whose default font is not fixed-cell.
- **Live input capture into replays.** `pollEvent()` finally makes this
  possible, and `REQUIREMENTS.md` currently lists it as out of scope.
  It should be picked up as its own piece of work once a backend exists,
  not folded into this one.
- **Runtime-loaded backends.** Addable later behind the unchanged
  `IGfxBackend` seam if a genuine need appears.
