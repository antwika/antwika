# Plan: adding an `input` library

A plan for adding input handling to Antwika: an abstraction over reading a
keyboard and a mouse, with the concrete input framework (SDL, raylib, ...)
chosen at build time and never named by Antwika's own source, and with live
input entering the engine only through the existing replay seam.

## Goals

- One abstraction that Antwika code reads input through, expressed only in
  terms of keys, buttons, positions and the actions an application binds
  them to.
- No file under `src/` may `#include` an SDL, raylib or other framework
  header, or link against one.
- Swapping the framework is a build configuration change, not a code
  change.
- The default build depends on nothing new, so CI keeps building and
  passing with no input framework installed.
- Input reaches the engine only through `IReplaySource`, so an interactive
  session records to a replay and that replay reproduces the same state.
- Reading input does not require opening a window.

The last goal is what separates this plan from
[`gfx-plan.md`](gfx-plan.md), and it drives the central decision below.
Scope is keyboard and mouse; see
[Deferred deliberately](#deferred-deliberately) for what that excludes.

## Why now

Nothing in the repository reads a keyboard or a mouse.
Every app is driven by a hand-authored replay script: `main.cpp` loads a
JSON file, wraps it in `replay::ReplaySource`, and `--record` merely
re-serializes the input it just consumed.

Three documents already name this work as the next step, and two of them
currently forbid it:

- `REQUIREMENTS.md`, Should have: "Live/interactive input capture ... should
  stay out of scope until the engine gains a live input source."
- [`gfx-plan.md`](gfx-plan.md), Deferred deliberately: "**Live input capture
  into replays.** `pollEvent()` finally makes this possible ... It should be
  picked up as its own piece of work once a backend exists, not folded into
  this one."
- `src/libs/gfx/include/antwika/gfx/WindowEvent.hpp`: "Keyboard and pointer
  input belong with the work that feeds live input into replays, which is
  out of scope until there is a live input source to record from."

This library is that live input source.

## Decision: input gets its own seam, not a ride on `gfx`

`antwika::input` does **not** depend on `antwika::gfx`, and `antwika::gfx`
does not depend on `antwika::input`.
Neither library appears in the other's `target_link_libraries`.

The alternative was to add key and pointer alternatives to
`gfx::WindowEventPayload`, which the gfx plan's header table originally
anticipated.
That was rejected: it would make every input consumer link a graphics
library and open a window before it could read a key, and an application
listening for input without a window is a legitimate thing to want.

That decision has one hard consequence, and it is the riskiest part of this
work.

### The frameworks pump one queue for everything

`SDL_PollEvent` drains a single process-global queue.
`Sdl3Backend::pollEvent()` already loops over it and silently discards every
event that is not `CloseRequested` or `Resized` -- which is to say, every
keyboard and mouse event SDL reports.

raylib has no queue at all, only global state, and the only thing that pumps
the window system is `EndDrawing()` inside `RaylibRenderer::present()`.

So two independently polling backends over one framework would starve each
other, non-deterministically, depending on which one polled first.

### The sharing happens in `backends/`, not in `src/`

A framework directory already owns that framework's global state and its
`find_package`.
It is the honest place to own its event queue too.

`backends/sdl3/` grows a second target, `antwika_input_backend`, and a
private, reference-counted `Sdl3Pump` shared by both targets.
The pump calls `SDL_PollEvent` once and routes each event into a window
queue or an input queue, so whichever subsystem polls first advances both.

The two library seams stay completely independent.
The framework's single queue is admitted in exactly one place, behind the
abstraction, rather than leaking upward as a rule both libraries would have
to cooperate on.

## Module layout

```
src/libs/input/    antwika::input          vocabulary, interfaces,
                                           InputError, device state,
                                           action mapping, replay bridge,
                                           NullInputBackend
backends/null/     + antwika_input_backend  selects NullInputBackend
backends/sdl3/     + antwika_input_backend  Sdl3InputBackend, shared pump
backends/raylib/   + antwika_input_backend  RaylibInputBackend, state diff
```

`backends/` sits at the repository root, outside `src/`, for the reason
[`gfx-plan.md`](gfx-plan.md) gives and this plan repeats under
[Coverage](#coverage-the-same-constraint-the-same-answer).

Every backend directory builds a target named `antwika_input_backend`,
aliased to `antwika::input_backend`, exactly as it already does for
`antwika_gfx_backend`.
Only one is ever added to the build, so the fixed name never collides, and
apps link `antwika::input_backend` without knowing which framework they got.

## `antwika::input`

Contents of `src/libs/input/include/antwika/input/`:

| Header | Purpose |
| --- | --- |
| `Key.hpp` | `enum class Key`, `kKeyCount`, and its string converters |
| `MouseButton.hpp` | `enum class MouseButton` and its string converters |
| `KeyModifiers.hpp` | which of shift, control, alt and super were held |
| `Position.hpp` | a point in the backend's own surface coordinates |
| `InputEvent.hpp` | the raw edge events and the variant over them |
| `InputCapabilities.hpp` | which devices a backend can report at all |
| `IInputBackend.hpp` | the seam: name, capabilities, event pump |
| `SelectedInputBackend.hpp` | declares the build-time-selected factory |
| `NullInputBackend.hpp` | headless backend that reports nothing |
| `Keyboard.hpp`, `Mouse.hpp`, `InputState.hpp` | device state, folded |
| `Binding.hpp`, `ActionMap.hpp` | inputs bound to action names |
| `Events.hpp` | the `input.*` event name constants |
| `IInputEventCodec.hpp`, `InputEventCodec.hpp` | the event bridge |
| `LiveInputSource.hpp` | the live `IReplaySource` |
| `StopOnKeySource.hpp` | decorator appending `engine.stop` on a chosen key |
| `InputError.hpp` | the one exception type for this failure category |

The library depends on `antwika::log` for backend diagnostics, and -- only
for the replay-bridge headers -- on `antwika::event`, `antwika::replay`,
`antwika::time` and `antwika::engine`.
It does not depend on `antwika::gfx`.

`Position` duplicates `gfx::Point` field for field.
That is the deliberate price of the independence decision above, and its doc
comment says so, along with the fact that input positions are in whatever
surface coordinates the backend reports and are not a window coordinate
until an application decides they are.

### Interface sketch

```cpp
struct KeyPressed  { Key key; KeyModifiers modifiers; bool repeat = false; };
struct KeyReleased { Key key; KeyModifiers modifiers; };

struct PointerMoved          { Position position; };
struct PointerButtonPressed  { MouseButton button; Position position;
                               KeyModifiers modifiers; };
struct PointerButtonReleased { MouseButton button; Position position;
                               KeyModifiers modifiers; };
struct PointerScrolled       { std::int32_t horizontal;
                               std::int32_t vertical; };

using InputEvent = std::variant<KeyPressed, KeyReleased, PointerMoved,
                                PointerButtonPressed,
                                PointerButtonReleased, PointerScrolled>;

class IInputBackend
{
public:
    virtual ~IInputBackend() = default;

    /**
     * @brief Get the backend's name, for logs and diagnostics.
     * @return A stable identifier, e.g. "null".
     */
    [[nodiscard]] virtual std::string_view name() const = 0;

    /**
     * @brief What this backend can report at all.
     *
     * Not every source has both devices.
     * A keyboard-only backend says so here, rather than being required to
     * pretend it has a pointer whose events never arrive.
     *
     * @return The devices this backend reports. Never all false.
     */
    [[nodiscard]] virtual InputCapabilities capabilities() const = 0;

    /**
     * @brief Take the next event reported since the last call.
     *
     * Never blocks: an empty queue is reported, not waited on.
     *
     * @return The next event, or nullopt when none is pending.
     */
    [[nodiscard]] virtual std::optional<InputEvent> pollEvent() = 0;
};
```

Every event is an **edge**: a press, a release, a move, a scroll notch.
State is derived above the seam and never reported by it.

That is what lets an event-queue framework like SDL and a state-polling
framework like raylib implement the same interface, rather than the
interface being one of them with extra steps.
It is also what keeps a replay small and honest: only the edges are external
input and get persisted, and the held state is regenerated deterministically
by folding them, which is the same rule `engine.tick` follows.

Deliberately absent: any notion of which window an event arrived at.
See [Deferred deliberately](#deferred-deliberately).

### The selection seam

`antwika::input` declares one function and never defines it, mirroring
`gfx::makeSelectedBackend()`:

```cpp
// src/libs/input/include/antwika/input/SelectedInputBackend.hpp
namespace antwika::input
{
    /**
     * @brief Create the input backend chosen at build time.
     *
     * Declared here but deliberately not defined here: the definition
     * comes from whichever backend under backends/ was selected via
     * ANTWIKA_INPUT_BACKEND.
     * That is what keeps every concrete input framework out of src/
     * entirely.
     *
     * @param logger Receives the backend's diagnostics.
     * @return The selected backend, never null.
     * @throws InputError If the underlying framework failed to initialise.
     */
    [[nodiscard]] std::unique_ptr<IInputBackend> makeSelectedInputBackend(
        ILogger &logger);
} // namespace antwika::input
```

A declaration creates no link-time dependency, so `antwika::input` stays a
leaf and `antwika::input_backend` depends on it, not the other way round.

### Device state

`Keyboard` and `Mouse` fold the edge stream into what application code
actually asks about, and `InputState` is the aggregate an app holds:

```cpp
class InputState final
{
public:
    /**
     * @brief Clear this tick's edges, keeping what is still held down.
     */
    void beginTick() noexcept;

    /**
     * @brief Fold one event into the state.
     * @param event The event to apply.
     */
    void apply(const InputEvent &event);

    [[nodiscard]] const Keyboard &keyboard() const noexcept;
    [[nodiscard]] const Mouse &mouse() const noexcept;
};
```

`Keyboard` offers `isDown()`, `wasPressed()`, `wasReleased()` and
`modifiers()`, backed by a `std::bitset<kKeyCount>` -- which is why `Key`
values are contiguous from zero and why `keyIndex()` exists.
`Mouse` offers `position()`, `delta()`, the same three button queries, and
`scroll()`.

`wasPressed()` means "during this tick", which is why `beginTick()` is the
caller's responsibility rather than something a reader clears as a side
effect.
An edge that cleared itself on the first read would give a different answer
to the second reader, and an application with two systems asking about the
same key would then depend on which ran first.

### Action mapping

`ActionMap` is the reuse win across apps, and what makes controls
rebindable:

```cpp
using Binding = std::variant<Key, MouseButton>;

class ActionMap final
{
public:
    /**
     * @brief Bind an input to an action name.
     * @param action The application's name for the action.
     * @param binding The key or button that triggers it.
     * @param required Modifiers that must also be held.
     * @throws InputError If action is empty.
     */
    void bind(std::string action, Binding binding,
              KeyModifiers required = {});

    [[nodiscard]] bool isActive(std::string_view action,
                                const InputState &state) const;
    [[nodiscard]] bool wasTriggered(std::string_view action,
                                    const InputState &state) const;
};
```

One action holds several bindings, so WASD *and* the arrow keys can drive
the same movement.
Bindings are stored in a `std::map` keyed by action name, so iteration order
is deterministic rather than dependent on a hash seed.

## Determinism: input enters through `IReplaySource`, and nowhere else

`replay::EngineLoop` asks `IReplaySource::eventsFor(tick)` exactly once per
tick, with a monotonically increasing tick starting at zero.
A polling implementation is explicitly anticipated: that interface is
neither `const` nor `noexcept`.
It is the whole entry point, and this library adds no second one.

```
IInputBackend::pollEvent()
        |
        v
LiveInputSource::eventsFor(tick)      encodes each edge as event::Event
        |
        v
TickedEventDispatcher                 stamps the tick
        |
        +-----------------+-----------------------+
        v                 v                       v
  app tick sinks      StopSignal           TickEventRecorder
  translate input                          what --record writes
  into state changes
```

Four consequences, because they decide where code goes:

**Recording needs no new code.**
`saveReplayFile()` filters a deny-list of self-generated names --
`engine.tick`, plus an app's own startup announcement -- and writes
everything else.
Anything a source returns is therefore recorded automatically, with the
right tick.
The rule is simply that no `input.*` name may ever be added to an app's
`kSelfGeneratedEventNames`.

**Translation from input to application meaning belongs downstream of the
recorder**, in a tick sink, not in the source.
A click becomes "toggle the cell at (3, 4)" inside the tick path, so the
replay stores the click and regenerates the toggle.
Doing it in the source would persist the derived event instead, which
violates the requirement that a replay persist only external input.

**`engine.stop` is genuine input** and has to come from the source, or a
replay will not stop at the tick the live run did.
Hence `StopOnKeySource`, a decorator over any `IReplaySource` that appends
`engine.stop` when it sees a key-down for the configured key.
A decorator rather than a flag on `LiveInputSource`, per the project's
preference for composition over modifying an already-tested class.

**A `--replay` run must not attach a live source**, or input arrives twice.

### The wire format

`Events.hpp` declares `input.key_down`, `input.key_up`,
`input.pointer_move`, `input.pointer_down`, `input.pointer_up` and
`input.pointer_scroll`.

This makes `antwika::input` the first library besides `antwika::engine` to
own an application-visible event namespace.
That is a precedent worth setting knowingly, and it is why the codec lives
here rather than in each app: one place owns both the encoder and the
decoder, so the two cannot drift.

Payloads are JSON with **symbolic** names, never platform scancodes:

```json
{"key": "Escape", "shift": false, "control": false, "alt": false,
 "super": false, "repeat": false}
```

```json
{"button": "Left", "x": 412, "y": 118, "shift": false, "control": false,
 "alt": false, "super": false}
```

A raw SDL or raylib keycode would make a replay backend-specific, breaking
the guarantee that a session recorded under one backend reproduces under
another -- the same guarantee rendering keeps by being write-only.
`toString(Key)` and `keyFromString()` follow the converter idiom already
used by `log::Level` and `holdem::Stage`.

Decoding reuses `replay::parseAndValidatePayload<InputError>` with a
per-kind validator cached in a function-local `static`, exactly as
`life::BoardSink` does for `life.toggle_cell`.

`IInputEventCodec` exists as an interface with one implementation so that
`LiveInputSource` can be unit-tested against a mock codec, which the style
guide endorses for precisely this case.

## Build-time selection

The root `CMakeLists.txt` gains a sibling to `ANTWIKA_GFX_BACKEND`:

```cmake
set(ANTWIKA_INPUT_BACKEND "${ANTWIKA_GFX_BACKEND}" CACHE STRING
    "Input backend to compile and link, named after a backends/ subdirectory")
```

Defaulting to the graphics choice means one flag drives both in the common
case.
The default build stays `null`/`null` and adds no dependency, while
`-o gfx_backend=sdl3` gets SDL input for free.
It can still be overridden: `gfx=sdl3, input=null` is legal, and so is
`gfx=null, input=sdl3` for an application that wants input without a window.

`backends/CMakeLists.txt` changes from adding one directory to adding the
deduplicated set of selected ones:

- build a unique list of `{ANTWIKA_GFX_BACKEND, ANTWIKA_INPUT_BACKEND}`;
- `FATAL_ERROR` on any name that is not a directory, keeping the existing
  "Available: ..." message built by globbing `*/CMakeLists.txt`;
- `FATAL_ERROR` when the two differ and neither is `null`, because two
  frameworks would fight over one OS event queue;
- `add_subdirectory()` each selected directory exactly once.

That third check is worth having rather than trusting: the failure it
prevents is silent, framework-dependent event loss at runtime, and a
configure-time message can explain itself.

Each `backends/<name>/CMakeLists.txt` then guards its two targets on whether
it was selected for that subsystem, so a directory selected for input only
does not build a graphics backend.

`conanfile.py` gains `"input_backend": ["auto", "null", "sdl3", "raylib"]`
defaulting to `auto`, resolved to the `gfx_backend` value in `configure()`
and passed to CMake as the `ANTWIKA_INPUT_BACKEND` cache variable in
`generate()`.
Its `requirements()` already pulls SDL3 or raylib per framework; the
condition widens to "either option selects it".

Because selecting a backend changes the dependency graph, the existing
per-configuration lockfiles are regenerated once.
A mixed configuration would need its own lockfile, and only earns one if CI
ever pins one.

## Coverage: the same constraint, the same answer

CI enforces 100% line, function and branch coverage on the GNU leg, over
`--filter 'src/.*'`.

Input backends can never execute there: CI has no display, no SDL and no
raylib.
So they live under `backends/`, outside the gcovr filter, exactly like the
graphics backends and for exactly the same reason.
No new `GCOVR_EXCL_LINE` is needed anywhere.

That split decides what goes where.
Everything testable without a framework -- the vocabulary, the string
converters, the state folding, the action mapping, the codec,
`LiveInputSource`, `StopOnKeySource` and `NullInputBackend` -- lives under
`src/libs/input/` and is held to 100% like every other library.
Only framework polling lives under `backends/`, verified by the conformance
suite instead.

`scripts/check_line_length.py` and `scripts/check_one_sentence_per_line.py`
already glob `backends/**`, so neither needs changing this time.
`scripts/check_unused_test_doubles.py` scans `src/` only, so every new mock
and fake must be included by at least one test.

## Verifying backends: a conformance suite

`src/libs/input/tests/conformance/` builds an INTERFACE target
`antwika::input::tests::conformance` exporting a typed GoogleTest fixture
parameterised over a `Traits::create(ILogger &)` factory -- the same shape
as `GfxBackendConformance`, including its poll-count drain guard and its
`GTEST_SKIP()`-on-capability idiom.

No portable way exists to *simulate* a keypress, so the suite asserts
invariants rather than reactions:

- `name()` is not empty, and `capabilities()` reports at least one device.
- `pollEvent()` drains to an empty queue rather than blocking, so a caller
  draining it between ticks terminates.
- It still drains to empty when polled repeatedly, which means a backend
  reading live state must latch what it already reported.
  This is the invariant `RaylibWindow::takePendingEvent()` already documents
  for resizes, and it is the one a state-diffing input backend is most
  likely to get wrong.
- No events arrive with nobody touching anything.
- A backend reporting no pointer never emits a pointer event.

What is deliberately not asserted matters as much: nothing requires that any
event ever arrive, because a headless or unfocused backend is entitled to
report nothing, and requiring otherwise would force an honest backend to
lie.

CI instantiates the suite against `NullInputBackend`, inside the coverage
gate.
`backends/sdl3/tests/` and `backends/raylib/tests/` instantiate the same
suite unmodified, run by the existing `gfx-backends` matrix job under
`xvfb-run`.
That job's "the conformance suite actually ran" check must be extended to
the new binaries, so a suite that skipped everything cannot pass while
proving nothing.

A backend is done when it passes the suite unmodified -- which is also the
strongest available check that the abstraction is not quietly shaped around
one framework.

## Phases

Each phase ends with a green build and test run, and is done in its own git
worktree.

### Phase 1 -- the vocabulary and the seam

`src/libs/input/` with `Key`, `MouseButton`, `KeyModifiers`, `Position`,
`InputEvent`, `InputCapabilities`, `IInputBackend`, `InputError`,
`NullInputBackend`, the `SelectedInputBackend.hpp` declaration, and
`MockInputBackend` under `tests/mocks/`.
Add `add_subdirectory(input)` to `src/libs/CMakeLists.txt`.
Full unit tests, 100% coverage, no new dependency.
Nothing else in the repository changes yet.

### Phase 2 -- device state and action mapping

`Keyboard`, `Mouse`, `InputState`, `Binding` and `ActionMap`.
Pure value-level logic, entirely unit-testable by feeding hand-built
`InputEvent`s.
This is the phase that makes the library useful to an application before any
real backend exists.

### Phase 3 -- selection machinery and conformance

`ANTWIKA_INPUT_BACKEND`, the `backends/CMakeLists.txt` multi-directory
rework, the `input_backend` Conan option, `backends/null/`'s second target,
and the conformance suite instantiated against `NullInputBackend`.
This locks the contract down before a real backend can bend it.

### Phase 4 -- the replay bridge

`Events.hpp`, `IInputEventCodec` and `InputEventCodec` with its schemas,
`LiveInputSource`, `StopOnKeySource`, and a `FakeInputBackend` scripted
queue under `tests/fakes/`.

Ends with the library-level determinism proof: drive a run from the fake
backend through `LiveInputSource`, record it, replay the recording through
`ReplaySource`, and assert the final state is identical.
That is the same style of proof `ReplayDeterminismTest` already uses, and it
is the claim this whole library rests on.

### Phase 5 -- `backends/sdl3` input

The shared `Sdl3Pump`: move `SDL_Init`/`SDL_Quit` into it, reference-counted
through a `weak_ptr` in a function-local static, with `Sdl3Backend` and the
new `Sdl3InputBackend` each holding a `shared_ptr`.
It drains `SDL_PollEvent` once and routes into two bounded queues.

Bounded because an application polling only one subsystem would otherwise
grow the other's queue without limit; a documented cap that drops the oldest
is the right trade for a queue nobody is reading.

Plus the SDL keycode to `Key` and SDL button to `MouseButton` tables.
The existing graphics conformance suite must still pass unmodified after the
refactor, which is the safety net for touching working code.

### Phase 6 -- `backends/raylib` input

The second backend is where the abstraction is actually proven.
raylib reports no events, so `RaylibInputBackend` diffs `IsKeyDown`,
`IsMouseButtonDown`, `GetMousePosition` and `GetMouseWheelMove` against what
it last reported and synthesises the edges -- the same latching technique
`RaylibWindow` already uses for resizes.

Two caveats to document rather than hide: raylib needs a window to exist
before it reports anything, and its input state only advances when
`EndDrawing()` runs, so events arrive only for an application that presents
frames.

Expect this phase to force small changes to `IInputBackend`, and treat the
interface as unstable until it lands.
An interface validated against one framework is that framework with extra
steps.

### Phase 7 -- the applications

`src/apps/gfx_demo/`: `DemoLoop` takes an `IInputBackend &` alongside its
`IGfxBackend &`, drains both, folds input into an `InputState`, and drives
the scene through an `ActionMap` -- "quit" bound to Escape, "move_left" to
both A and Left, and so on.
`DemoScene` stays stateless and takes an offset.
Tests extend `DemoLoopTest` against `MockInputBackend`, with no framework
involved.

`src/apps/life/`: mouse-driven cell toggling.
A new `PointerToggleSink` holds the `World &` and the grid geometry, decodes
`input.pointer_down`, and toggles the cell under the cursor -- a new sink
beside `BoardSink` rather than a change to it.

This needs something to click on, so it also delivers the render system that
is [`gfx-plan.md`](gfx-plan.md)'s Phase 6, the one phase of that plan still
outstanding.
If that proves too large to carry here, it is the natural split point: the
render system can land first, on its own.

Ends with the end-to-end proof: record a live session under sdl3, replay it
under the null backends, and assert the final `World` state is identical.
Then the blog post, per the project's usual practice of writing up a design
after the fact.

## Documentation and requirements to update

- `REQUIREMENTS.md` loses the Should-have deferring live input capture and
  the two Won't-haves that forbid it (live capture, and `antwika::gfx`
  reporting input).
  Those move rather than being quietly ignored.
- `REQUIREMENTS.md` gains Must-haves: input access must go through a
  backend-agnostic abstraction, and no file under `src/` may reference a
  concrete input framework; the input backend must be selected at build
  time; a headless input backend must exist; input must reach the engine
  only through `IReplaySource`, so a recorded interactive session replays to
  the same state; input events must be persisted with symbolic key names
  rather than platform scancodes; a bad input payload must raise one
  specific catchable error type; polling an input backend must reach an
  empty queue; and a backend must declare which devices it can report and
  never report others.
- `REQUIREMENTS.md` gains Won't-haves: no text or IME input, no cursor
  capture, no gamepad, no touch, no window attribution of input events, and
  no runtime-loadable input backends.
- [`gfx-plan.md`](gfx-plan.md)'s "Live input capture into replays" deferral
  points here.
- `src/libs/gfx/include/antwika/gfx/WindowEvent.hpp`'s "out of scope until
  there is a live input source" comment is now stale, and should say that
  input travels through `antwika::input` instead, and why.
- `CLAUDE.md` and `README.md` gain an `antwika::input` paragraph and the
  `ANTWIKA_INPUT_BACKEND` / `-o input_backend=` build instructions.
- `.github/workflows/build.yml` gains `antwika_input_tests` in its expected
  binary list, and the new backend test binaries in the `gfx-backends` job's
  "suite actually ran" check.
- `.vscode/tasks.json` and `scripts/select_gfx_backend.sh` cover the
  graphics backend only, and should either set both or gain an input
  equivalent.

## Deferred deliberately

- **Which window an event arrived at.**
  `IInputBackend` reports no surface id, because input is independent of
  `gfx` and cannot name a window without inventing a second, unenforceable
  id vocabulary alongside `WindowId`.
  A single-surface application is unaffected, which is every application
  here.
  Multi-window input routing needs a contract between the two seams that is
  worth designing against a real two-window application rather than
  guessing at now.
- **Text and IME input.**
  A character stream is genuinely different from a keycode stream --
  composition, UTF-8, `SDL_EVENT_TEXT_INPUT` versus keycodes -- and belongs
  with the first application that needs a text field.
- **Cursor control**: hiding, warping, and relative capture for mouse-look.
  These write to a window rather than read input, so they land on `IWindow`,
  not here.
- **Frame pacing.**
  `EngineLoop` consults no clock and busy-spins, so an interactive
  application would consume input at millions of ticks per second and record
  an enormous replay.
  The determinism-safe place for a wait is a `PacedReplaySource` decorator
  that emits nothing, paired with an `ISleeper` and `FakeSleeper` in
  `antwika::time`.
  That belongs in `replay` and `time` rather than here, and is its own small
  piece of work.
- **A `backends/stdin/` keyboard backend.**
  The clearest payoff of not tying input to `gfx`: a raw-mode terminal
  backend reporting no pointer, letting a terminal-printed application like
  `apps/life` take live input with no window at all.
  It needs a separate Windows implementation, so it is a Could-have rather
  than scope here.
- **Gamepads, touch, and multiple keyboards.**
  Each would widen the seam, and none is needed by an application that
  exists.
- **Runtime-loaded backends.**
  Rejected for graphics for reasons that apply here unchanged, and addable
  later behind the unchanged `IInputBackend` seam if a genuine need appears.
