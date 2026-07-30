# Plan: input for `antwika::ui`

A plan for making a `antwika::ui` button clickable: hover and press appearance a widget works out for itself, an activation the caller can act on, and a click that replays exactly in an app driven by the tick loop.

**Status: proposed.**
Nothing below has been built.

## Context

`antwika::ui` is display-only by decision, not by omission.
[`docs/ui-plan.md`](ui-plan.md) deferred "all pointer and keyboard input, hit-testing and interaction" with a reason that was true when it was written: `antwika::input` was vocabulary-only, there was no input backend under `backends/`, and `NullInputBackend::pollEvent()` always returned nullopt, so no button could have been clicked in any build that existed.

That is no longer the case.
`src/libs/input/` now folds device state (`InputState`, `Keyboard`, `Mouse`), `backends/sdl3` and `backends/raylib` both report edges, and two applications already turn a click into application meaning inside the tick path: `game::GridSink` and `life::PointerToggleSink`.

What the earlier plan left in place is the seam this work plugs into.
`ButtonState` exists and is documented as the thing "whatever gains a pointer later has a place to report ... without any signature changing".
The layout arena already holds every widget's final rectangle in `Node::arranged`, which is exactly what a hit-test needs and is already computed.

`REQUIREMENTS.md:105` is the Won't-have this work retires:

> `antwika::ui` won't read a pointer or a keyboard, and no UI interaction will reach the engine or a replay, in its current scope; a button is told how it should look rather than working it out.

The "in its current scope" clause is the licence to change it, and this document is the scope that replaces it.

## Goals

- A button reports that it was activated, and shows a hovered and a pressed appearance it worked out itself.
- Interaction is resolved against the *same frame's* layout, so what a click hit is what was on screen when it was clicked.
- `antwika::ui` still depends on `antwika::gfx` and nothing else -- not `antwika::input`, not `antwika::event`, not `antwika::replay`.
- `antwika::ui` still holds nothing between frames and still reads nothing outside its arguments.
- Interaction, like the picture, is a value: assertable with `EXPECT_EQ`, with no new mock and no new test double.
- A UI click in an engine-driven app replays exactly, with **no new event name persisted** -- the click is stored and the activation is regenerated, the rule `GridSink` and `PointerToggleSink` already follow.
- 100% line, function and branch coverage, with no `GCOVR_EXCL_LINE` shape beyond the two the repository already documents.
- Every existing caller of `antwika::ui` compiles unchanged and draws exactly what it drew before.

Explicit non-goals: keyboard input and focus, text entry, widgets that carry a value, clipping and scrolling, z-order and popups, and any retained interaction state anywhere in the library.

## Decision: one new pure stage, between layout and flatten

```
Context (immediate-mode calls)  ->  LayoutTree     flat arena of Node
LayoutTree + canvas             ->  layout()       measure, then arrange
LayoutTree + pointer            ->  resolve()      hit-test, then style     <- new
LayoutTree                      ->  flatten()   -> DrawList (a value)
DrawList + IRenderer            ->  paint()        write-only translation
```

`resolve()` is the whole of the new behaviour.
It runs after `layout()`, because a hit-test needs `arranged`, and before `flatten()`, because what it decides is a background colour that flattening then emits.
`layout()` and `flatten()` are untouched, `paint()` is untouched, and the drawing side stays exactly as write-only as it was.

### Why not `if (ui.button("ok"))`

The classic immediate-mode idiom returns the click from the call that declares the widget.
It cannot work here, and the reason is the same one that made the layout deferred in the first place: at the moment `button()` is called, nothing knows where the button will end up.

There is a well-known workaround -- hit-test against *last* frame's rectangle for the same widget -- and it is worth writing down why it is rejected rather than leaving it to look like an oversight:

- It makes the library retain a whole frame's layout between frames, which is precisely the property the current design does not have.
- It resolves a click against a canvas size that is not the one the click will be drawn against, so a resize, a scale change or a conditional layout silently mis-attributes the first click after it.
- It makes widget identity load-bearing *across* frames, so a layout that declares a different set of widgets this frame quietly re-points last frame's rectangles.

Instead, the caller names the widget when it declares it, and asks what happened after `finish()`:

```cpp
const auto frame = ui.finish();

if (frame.interactions.activated == kZoomIn)
{
    camera.zoomIn();
}
```

Everything is resolved against one canvas, one layout and one pointer position, all from the same frame.

### Activation is on press, and so the library retains nothing at all

A click in most toolkits is press-then-release-on-the-same-widget, which requires remembering, between frames, which widget captured the press.
That state has to live somewhere, and in this repository "somewhere" is a loaded question: anything a click is interpreted against must be regenerable from replayed input, which is the lesson of [`blog/013-the-camera-is-simulation-state.md`](../blog/013-the-camera-is-simulation-state.md).

So a button activates **on the press**.
`game::GridSink` already made the same call for the same kind of reason, and says so:

> Middle-button drag rather than left, so that placement can stay on the press: a left-drag pan would need a "moved more than N pixels, so that was a drag" rule, which moves placement to the release and invents a threshold nothing else here justifies.

The consequence is worth stating plainly: with activation on press, `finish()` is a pure function of the declarations, the canvas and this frame's pointer, and `Context` keeps its documented property of holding nothing between frames.
No capture, no id stability requirement across frames, nothing for a replay to regenerate beyond the pointer state an app is folding anyway.

The cost is that a press cannot be cancelled by sliding off the button before letting go.
`ButtonState::Pressed` is still shown while the button is held over the widget, which is honest -- it says "this is the one you are on", not "this is about to fire".
Release-to-activate is written up in [Deferred deliberately](#deferred-deliberately) with the exact price of adding it later.

### The pointer arrives as an argument

```cpp
Context(Size canvas, Theme theme, Pointer pointer = {});
```

`antwika::ui` gains no dependency.
It does not link `antwika::input`, does not name `input::InputEvent`, and never sees an edge: an application folds edges into `input::InputState` -- which is what that class is for -- and hands the result across as a plain value.

That keeps two properties that are otherwise lost.
`antwika::ui` stays a leaf on `antwika::gfx`, so a test drives interaction by writing a `Pointer` literal, with no input library, no backend and no fake in sight.
And `REQUIREMENTS.md:54` stays true word for word -- "no file under `src/libs/ui/` may read a clock, a pointer, a keyboard or any state outside its arguments" -- because the pointer *is* an argument.
The requirement was phrased that way deliberately and does not need weakening.

`Pointer` uses `gfx::Point`, not a duplicate of `input::Position`.
`antwika::input` duplicates that struct precisely so it need not depend on `antwika::gfx`; `antwika::ui` already depends on `antwika::gfx` and already speaks its `Point`, `Rect`, `Size` and `Color`, so a third copy would buy nothing.
The application converts, field for field, the way `game::GridSink::asPoint()` already does.

The default argument matters more than it looks.
`Pointer{}` reports no pointer at all, so every existing call site keeps its current behaviour exactly, and the display-only use of the library remains a first-class one rather than a degraded interactive one.

## The vocabulary

Five new public types, all values.

```cpp
// WidgetId.hpp
struct WidgetId
{
    std::uint64_t value = 0;

    [[nodiscard]] bool operator==(const WidgetId &other) const = default;
};

inline constexpr WidgetId kNoWidget{};
```

```cpp
// Pointer.hpp
struct Pointer
{
    // Absent when nothing has reported a pointer, which is also what a
    // build with no pointer device reports for the whole run.
    std::optional<Point> position{};

    // Whether a button is being held, and whether it went down this
    // frame.  Both are edges an application already folds.
    bool down = false;
    bool pressed = false;

    [[nodiscard]] bool operator==(const Pointer &other) const = default;
};
```

```cpp
// Interactions.hpp
struct Interactions
{
    WidgetId hovered = kNoWidget;
    WidgetId activated = kNoWidget;

    // Whether the pointer is over anything this UI filled in.
    bool pointerOverUi = false;

    [[nodiscard]] bool operator==(const Interactions &other) const = default;
};
```

```cpp
// Frame.hpp
struct Frame
{
    DrawList commands;
    Interactions interactions;
};
```

```cpp
// ButtonSpec.hpp
struct ButtonSpec
{
    WidgetId id = kNoWidget;
    Sizing width = kFit;

    // Set to say how the button must look regardless of the pointer.
    std::optional<ButtonState> state{};
};
```

`activated` is a single id rather than a list.
One pointer produces one press, and one press lands on one topmost widget, so a second activation in a frame is not expressible -- which is cheaper to specify and cheaper to cover than a vector that can only ever hold zero or one element.

### Why the id is symbolic and caller-supplied

Declaration order is the obvious alternative, and it is deterministic within a frame, so for *appearance* it would be enough.
It is not enough for what crosses back to the caller.
An index shifts the moment a layout gains a conditional widget, and the value an application compares against, stores in its own state, or writes into a log would silently start meaning a different button.
A symbolic id is also what a test asserts, and what reads correctly at the call site.

`WidgetId` is a strong type over `std::uint64_t`, following `gfx::WindowId` and `ecs::Entity`.
Not a string: that is an allocation per interactive widget per frame in a library that is otherwise arithmetic, and hashing one invites a collision this library has no error type to report -- there is deliberately no `UiError`, and this work adds none.

Duplicate ids are legal and mean "these are the same widget": the topmost one is hovered and activated, and every one of them takes the resolved appearance.
Nothing is checked, because nothing is wrong.

## Hit-testing

The candidate is the **last** node, in arena order, whose arranged rectangle contains the pointer and which carries an id.

Ascending index is paint order, so descending index is front-to-back.
That mirrors the existing passes rather than introducing a new ordering concept: measure descends, arrange ascends, flatten ascends, hit-test descends.
And because layout guarantees containment -- a child never escapes its parent, which is why overflow shrinks proportionally -- the deepest hit is also the frontmost, so one loop answers both questions at once.

A button's spacers and its label carry no id, so they are skipped and the button's own container wins.
That is not a special case in the hit-test; it falls out of only interactive nodes carrying an id.

Containment is half-open, stated once and tested at all four edges:

```
x >= origin.x && x < origin.x + width
y >= origin.y && y < origin.y + height
```

The arithmetic is done in `std::int64_t`.
`Rect::origin` is a `Point` of `std::int32_t` and `Rect::size` is a `Size` of `std::uint32_t`, so `origin.x + width` is exactly the shape that overflows a 32-bit signed addition on a large-but-legal layout.
A zero-width or zero-height rectangle therefore contains nothing, which is the behaviour the degenerate cases in `docs/ui-plan.md` already produce for a collapsed container.

### What "over the UI" means

`pointerOverUi` is true when the pointer is inside the arranged rectangle of **any node with a background**, whether or not it has an id.

A filled panel visually covers whatever the application drew underneath, so a click that lands on it must not also reach the world behind it.
A node that draws nothing covers nothing and cannot swallow anything -- a growing spacer in a transparent row is not a wall.
That rule is what lets an application put a toolbar over its scene and keep its existing world clicks working, and it is one boolean rather than an input-capture protocol.

## Resolving appearance

`Node` gains two fields:

```cpp
WidgetId id = kNoWidget;

// The three colours a resolved state picks between.
// Absent when nothing is to be resolved, which includes every
// non-interactive node and any button whose caller forced its state.
std::optional<Interactive> style{};
```

`Interactive` is a private `detail` struct of three colours, taken from the theme when the widget is declared, so `resolve()` needs no `Theme` argument and stays a pure function of the tree and the pointer.

`Context::button()` keeps doing exactly what it does today when the caller forces a state -- it bakes `buttonFill(theme, *spec.state)` into `background` and leaves `style` unset.
Otherwise it sets `style` and leaves `background` for `resolve()` to write:

| Condition | Appearance |
| --- | --- |
| `spec.state` is set | that state, whatever the pointer is doing |
| `id == interactions.hovered` and `pointer.down` | `Pressed` |
| `id == interactions.hovered` | `Hovered` |
| otherwise | `Idle` |

So the existing "the caller decides" behaviour survives intact as an override, which is what a disabled button and a "it is this player's turn" highlight both need, and the new behaviour is what an un-forced button does.

`resolve()` is therefore two flat loops over the arena, in keeping with everything else in the library: one descending loop that finds `hovered` and `pointerOverUi`, and one ascending loop that writes a background for every node carrying a `style`.
`activated` is `pointer.pressed && hovered != kNoWidget ? hovered : kNoWidget`.

## Interaction in an engine app: where this must run

This is the part that is not about the library.

`antwika::ui` cannot break replay determinism on its own -- it is a pure function -- but an application can break it in three ways, so the plan pins the rules rather than leaving them to the app that gets there first.

```
IInputBackend::pollEvent()
        |
        v
LiveInputSource::eventsFor(tick)        input.* events, symbolic
        |
        v
TickedEventDispatcher  ------------->  TickEventRecorder   <- what --record writes
        |
        +--> UiSink       folds InputState, describes the UI, resolves it,
        |                 acts on interactions.activated, stores the DrawList
        |
        +--> GridSink     acts on the click only if the UI did not take it
        |
        +--> engine.tick  observers, including the renderer, which paints
                          the DrawList the UiSink already produced
```

**1. Describing and resolving the UI happens in a tick sink, downstream of the recorder.**
Never in the renderer, and never above the recorder.
A replay then carries the click and regenerates the activation, exactly as it carries a click and regenerates a laid path today.

**2. No `ui.*` event name is ever defined or persisted.**
An activation is not an event; it is a value read by the sink that computed it, in the same tick.
`kSelfGeneratedEventNames` needs no new entry, because there is nothing new to filter -- and adding one would be the smell that rule number one had been broken.

**3. The canvas the UI is laid out against is the configured window size, not the size a window reports.**
This is the rule `life::PointerToggleSink` already documents, and it matters more here: a hit-test is a function of the layout, and the layout is a function of the canvas, so resolving a recorded click against a differently-sized window resolves it to a different widget.
The window that carries a UI is therefore not resizable, which is already `WindowDesc`'s default.
The theme scale follows the same rule, since `scaleForCanvas()` changes every metric in the layout: it is derived from the same constant.

**4. The pointer comes from an `input::InputState` the sink owns.**
Held below the recorder, folded from the tick stream, so it is regenerated identically on replay -- the shape `GridSink` already uses, with its `InputState state;` and the comment saying why it lives there.
`Pointer::position` stays absent until the sink has seen its first `input.pointer_*` event, so a build with no pointer never reports one hovering at the origin.
(`input::Mouse` knows this internally and does not expose it; adding `isLocated()` there is a reasonable alternative, but it changes a second library for something one sink can fold for itself.)

**5. The description is built once per tick.**
The sink produces the `Frame`; the renderer paints `frame.commands`.
The picture and the hit-test come from one call, so they cannot drift -- the same argument as `life::layoutFor()`/`life::cellAt()` being one function shared by the scene and the sink.
The handover is a small application-owned holder, in the shape of `life::DragState`: written by the UI sink, read by the render observer and by the world sink.

**6. Sink order is pinned, and the world sink defers.**
The UI sink is registered before the world sink, and the world sink skips a press when the holder says the UI took it.
That ordering is part of the application's bootstrap and gets a test of its own: a click on the toolbar must lay no tile under it.

## The API delta

Everything below is additive except one return type.

```cpp
class Context final
{
public:
    // Was: Context(Size canvas, Theme theme);
    Context(Size canvas, Theme theme, Pointer pointer = {});

    // Was: void button(std::string_view, ButtonState = Idle, Sizing = kFit);
    void button(std::string_view text, ButtonSpec spec = {});

    // Was: [[nodiscard]] DrawList finish();
    [[nodiscard]] Frame finish();

    // Unchanged: theme(), row(), column(), panel(), label(), spacer().
};
```

`button()`'s two trailing arguments become one `ButtonSpec`, which is the `ContainerSpec` idiom the library already uses, and which is what keeps a third and fourth positional argument from accumulating.
`ui.button("ok")` is unchanged; `ui.button("ok", ButtonState::Hovered)` becomes `ui.button("ok", {.state = ButtonState::Hovered})`, which is one call site in the repository today.
`finish()` returns a `Frame`, so a caller that only draws writes `paint(renderer, ui.finish().commands)`.

## Files

### `src/libs/ui/include/antwika/ui/` -- new

| File | Purpose |
| --- | --- |
| `WidgetId.hpp` | the symbolic identity of an interactive widget, and `kNoWidget` |
| `Pointer.hpp` | what the caller reports about the pointer this frame |
| `Interactions.hpp` | what the frame's pointer did to the frame's widgets |
| `Frame.hpp` | the picture and the interactions, returned together |
| `ButtonSpec.hpp` | per-call button options, `ContainerSpec`'s counterpart |

### `src/libs/ui/src/` -- new and changed

| File | Change |
| --- | --- |
| `Resolve.hpp` / `.cpp` | new: `Interactions resolve(LayoutTree &tree, Pointer pointer);` and the private containment predicate |
| `Node.hpp` | gains `id` and `std::optional<Interactive> style` |
| `Context.hpp` / `.cpp` | the pointer, `ButtonSpec`, and `finish()` returning a `Frame` |

`Layout.cpp`, `Flatten.cpp`, `Painter.cpp`, `LayoutTree.cpp`, `Scope.cpp` and `Theme.cpp` are untouched.

### `src/libs/ui/tests/` -- new

`WidgetIdTest.cpp`, `PointerTest.cpp`, `InteractionsTest.cpp`, `ResolveTest.cpp`, `ResolveHitTest.cpp` and `ContextInteractionTest.cpp`, alongside the twelve that exist.
Still **no `mocks/` or `fakes/` directory**: a `Pointer` is a literal and an `Interactions` is compared with `EXPECT_EQ`, so this work adds no test double to any library, and `scripts/check_unused_test_doubles.py` has nothing new to look at.

## Phases

Each phase ends with a green build, a full `ctest` run and a clean coverage run, and is done in its own git worktree per `CLAUDE.md`.

### Phase 1 -- the vocabulary and `resolve()`

`WidgetId`, `Pointer`, `Interactions`, the `Node` fields and `resolve()`, tested over hand-built `LayoutTree`s.
No public API changes yet, so nothing outside the library can tell this landed.
This is where the hit-test, the half-open containment rule and the appearance table are pinned as exact values -- the largest new surface, and the one worth having provable before anything can drive it.

### Phase 2 -- `Context`

`ButtonSpec`, `Frame`, the pointer argument, and `button()` routed through `resolve()`.
`gfx_demo`'s one `ButtonState::Hovered` call site and the existing `ui` tests are updated for the two changed signatures.
At the end of this phase the library is usable and the demo still draws exactly what it drew before, which is the assertion `DemoSceneTest` already makes as a value.

### Phase 3 -- `gfx_demo` becomes clickable

`gfx_demo` gains `antwika::input`, folds an `InputState` in its own frame loop, and passes a `Pointer` to the `Context`.
Its panel gets two buttons that visibly do something -- a counter label and a colour it cycles.

`gfx_demo` is the right first consumer for the same reason it was the right first consumer of the library: it runs its own frame loop with no engine, no replay and no recorder, so there is nothing to get wrong about determinism yet, and `DemoLoopTest`/`DemoSceneTest` already drive it against `MockGfxBackend`.
This is the phase where a person can click a button, under `sdl3` and under `raylib`, and where the `null` build must still run and still draw nothing.

### Phase 4 -- a toolbar in `apps/game`, and the replay proof

`game::UiSink` (describe, resolve, act, store), a `game::UiOverlay` holder, `RenderSystem` painting it after the grid, and `GridSink` deferring to it.

The toolbar is three buttons -- `zoom -`, `zoom +`, `reset view` -- plus a label or two.
All three touch only the `Camera`, which [`blog/013`](../blog/013-the-camera-is-simulation-state.md) already established as simulation state folded from replayable input, so the demo exercises the whole path without inventing a new kind of application state to argue about.
The existing gestures are untouched: left-click still lays a path, right-click still drops a walker, middle-drag still pans.

Two tests carry this phase:

- a click on the toolbar changes the zoom and lays **no** tile under it;
- a `--record` run whose events are replayed reproduces the same camera and the same grid, with a recorded file that contains `input.*` events and nothing else.

### Phase 5 -- documentation

`REQUIREMENTS.md`, `CLAUDE.md`, `README.md` and this document, as listed below.
A `blog/` write-up is optional and, per `CLAUDE.md`, written afterwards.

## Testing and coverage

Everything under `src/libs/ui/` stays inside the gcovr filter and at 100% line, function and branch coverage on the GNU leg.
Branch checklist, written down now rather than chased later:

1. Pointer absent and present.
2. Pointer inside a widget, outside every widget, and outside the canvas entirely.
3. Containment at all four edges: on the leading edge (inside), on the trailing edge (outside), and one pixel beyond each.
4. A zero-size rectangle, which is hit by nothing.
5. Two overlapping id-bearing nodes: the later one wins.
6. An id-bearing node whose children carry no id: the container is hovered, not the label.
7. A hit on a node with no id and no background: nothing hovered, `pointerOverUi` false.
8. A hit on a node with a background and no id: `pointerOverUi` true, nothing hovered.
9. Every row of the appearance table, including the forced-state override with and without a pointer over the widget.
10. `pressed` with a widget under the pointer and `pressed` with none.
11. `down` without `pressed` (held from a previous frame) and `pressed` without `down` (a press and release inside one frame).
12. Duplicate ids: the topmost activates, both take the appearance.
13. A `Context` built with no pointer produces byte-identical commands to today.

No new `GCOVR_EXCL_LINE` shape is expected.
`resolve()` builds no `std::string` and no `std::vector`, so it has no unwind landing pad; `Context::button()` gains a branch, not an allocation.

The one determinism test that is not a unit test lives in `apps/game`: the record-and-replay round trip in Phase 4.
That is the assertion that the whole rule set in [Interaction in an engine app](#interaction-in-an-engine-app-where-this-must-run) is actually being followed, and it is the test to write first in that phase.

## Documentation and requirements to update

`REQUIREMENTS.md:105` is rewritten.
It becomes a Must-have set:

- A UI must resolve a pointer against the layout of the same frame it draws, so what a click hit is what was on screen when it was clicked.
- A UI widget must be identified by a symbolic id supplied by the caller, never by its position in the declaration order, since an id is what crosses back into application state.
- The canvas a UI is laid out and hit-tested against must be a configured constant rather than the size a window reports, so a recorded click resolves to the same widget under any backend and any window manager.
- Translating a UI activation into application meaning must happen downstream of the replay recorder, and no UI interaction may be persisted: a replay stores the click and regenerates what it activated.

`REQUIREMENTS.md` keeps a narrowed Won't-have, one line each:

- `antwika::ui` won't read a keyboard, hold focus, or offer any widget that carries a value.
- `antwika::ui` won't retain interaction state between frames, so a button activates on the press rather than on a release matched to it.
- `antwika::ui` won't read a clock, so nothing depends on how long a pointer rested or how quickly two clicks arrived.

`REQUIREMENTS.md:54` is left exactly as it is, and the plan is built to keep it true.

`CLAUDE.md`'s `antwika::ui` paragraph gains the interaction sentence and the rule about where a UI sink belongs in an app; `README.md` gains it in the library list and in the `gfx_demo` and `antwika_game` descriptions.
`docs/ui-plan.md` gains a line under [Deferred deliberately](ui-plan.md) pointing here, rather than being edited to pretend it planned this.

Nothing else needs touching.
No new dependency, so `conanfile.py` and the lockfiles are untouched; no new test binary, so `.github/workflows/build.yml` is untouched; the three checker scripts already glob what changes.

## Deferred deliberately

- **Release-to-activate, and pointer capture.**
  The price is exact: one `WidgetId` of state that must live outside the library, be threaded into the `Context` and back out of `finish()`, and be regenerated identically on replay -- so it becomes application state, subject to every rule the camera is.
  Worth doing when a UI has a destructive button somebody must be able to back out of; not worth it for a toolbar.
- **Keyboard input, focus, and tab order.**
  Focus is retained state by definition, and a tab order is a second traversal order to keep consistent with declaration order.
  Nothing here needs to type.
- **Text fields, sliders, checkboxes and anything carrying a value.**
  Each one needs retained state, and a slider needs a drag, which needs capture.
- **Double-click, long press, hover delay and tooltips.**
  All of them need a clock or a tick count, and `antwika::ui` reads neither, deliberately.
- **Drag and drop between widgets.**
  Capture plus a payload plus a drop target, none of which exist.
- **Scrolling, and therefore clipping.**
  Unchanged from `docs/ui-plan.md`: `IRenderer` has no scissor, so this is a `gfx` change and a `backends/` change before it is a `ui` change.
- **Z-order, popups and modals.**
  A popup needs a draw order independent of tree order *and* a hit priority independent of draw order.
  The hit-test here is the reverse of the paint order precisely because there is only one order to keep straight.
- **A cursor shape per widget.**
  `antwika::gfx` does not own a cursor, and asking it to would put a write back into the window abstraction that [`blog/012`](../blog/012-a-window-that-cant-talk-back.md) kept out.
- **Multi-pointer and touch.**
  One `Pointer` per frame.
  Nothing here prevents a second one, and nothing needs one.
- **An input-consumption protocol richer than one boolean.**
  `pointerOverUi` answers the only question an application has asked so far.
  Per-event consumption, hit-test layers and pass-through regions can be added when something wants them, and none of them changes the pure-stage shape.
- **Migrating `apps/poker`'s `TableScene` onto `antwika::ui`.**
  Still deferred, for the reason `docs/ui-plan.md` gives: it is a refactor of working, fully-tested code, and it should follow the library rather than ride along with a change to it.

## Verification

Per phase, from the repo root inside a dev container, in the phase's own worktree:

```sh
conan install . -of build \
  -pr:b=./profiles/build/${CONAN_PROFILE} \
  -pr:h=./profiles/host/${CONAN_PROFILE} \
  --build=missing -s build_type=Release --lockfile=conan.lock
cmake --preset conan-release
cmake --build build -j24
build/bin/antwika_ui_tests                  # fast inner loop, phases 1-2
ctest --test-dir build --output-on-failure  # before calling a phase done
```

Coverage, which is the gate that decides whether a phase is finished:

```sh
cmake --preset conan-coverage
cmake --build build-coverage -j24
ctest --test-dir build-coverage
gcovr --root . --filter 'src/libs/ui/.*' --exclude '.*/tests/.*' \
  --print-summary build-coverage
```

Then the `--filter 'src/.*'` run CI does, plus `python3 scripts/check_full_coverage.py --summary coverage-summary.json`.

Style gates:

```sh
python3 scripts/check_line_length.py
python3 scripts/check_one_sentence_per_line.py
python3 scripts/check_unused_test_doubles.py
```

Phases 3 and 4 need eyes as well as tests, under a backend that actually has a pointer:

```sh
conan install . -of build-sdl3 -o gfx_backend=sdl3 \
  -c tools.cmake.cmake_layout:build_folder_vars="['options.gfx_backend']" \
  -pr:b=./profiles/build/${CONAN_PROFILE} \
  -pr:h=./profiles/host/${CONAN_PROFILE} \
  --build=missing -s build_type=Release --lockfile=conan-sdl3.lock
cmake --preset conan-gfx_backend_sdl3-release
cmake --build build-sdl3 -j24
build-sdl3/bin/antwika_gfx_demo                        # phase 3
build-sdl3/bin/antwika_game --record /tmp/ui.replay    # phase 4, click the toolbar
build-sdl3/bin/antwika_game --replay /tmp/ui.replay    # same camera, same grid
ctest --test-dir build-sdl3 --output-on-failure
```

And the check that this did not quietly become one backend's shape: the same two runs under `raylib`, whose input backend reports a pointer and no keyboard, and under the default `null` backend, where nothing is ever hovered, nothing is ever activated, and both apps must still run and exit cleanly.
