# antwika::ui

`src/libs/ui/` — immediate-mode UI whose output is a value.

## What it is for

Describing a nestable layout of rows, columns, panels, labels and buttons, and turning it into a list of drawing commands plus the interactions the pointer produced.
Everything is laid out arithmetically from `gfx::textSize()` alone, so the library asks no backend to measure anything, and it is drawn through [`gfx`](gfx.md)'s rectangle and text calls.

## Key types

| Header | Type | Role |
| --- | --- | --- |
| `Context.hpp` | `Context` | The immediate-mode surface: `row()`, `column()`, `panel()`, `label()`, `button()`, `textField()`, `dropdown()`, `spacer()`, `finish()`. |
| `Scope.hpp` | `Scope` | A `[[nodiscard]]` guard returned by every container call, closing it in its destructor. |
| `Frame.hpp` | `Frame` | What `finish()` returns: `commands` (a `DrawList`), `interactions`, `rects` and `hoverTargets`. |
| `DrawList.hpp`, `DrawCommand.hpp` | `DrawList`, `FillRect`, `DrawText` | The picture, as plain comparable values. |
| `Painter.hpp` | `paint()` | The only thing in the library that touches an `IRenderer`. |
| `Pointer.hpp` | `Pointer` | The pointer, passed in as an argument; the default is no pointer at all. |
| `Interactions.hpp` | `Interactions` | The `hovered`, `activated` and `focused` `WidgetId`, the `edit` and `chosen` results, and whether the pointer is over anything the UI filled in. |
| `Keyboard.hpp` | `Keyboard`, `Key` | Key edges in arrival order, plus a `typed` view of the characters; defaults to none. |
| `TextFieldSpec.hpp` | `TextFieldSpec`, `TextEdit` | A field's characters, caret and focus going in; what happened coming out. |
| `DropdownSpec.hpp` | `DropdownSpec`, `OptionChoice` | A list's open/closed and selected state going in; what was chosen coming out. |
| `WidgetRects.hpp` | `WidgetRects` | One `gfx::Rect` per distinct id the frame named, with `find(id)`. |
| `HoverTargets.hpp`, `HoverTarget.hpp` | `HoverTargets`, `HoverTarget` | One target per named widget that works its own appearance out. |
| `HoverPointer.hpp` | `HoverPointer` | A position and nothing else — no `down`, no `pressed`. |
| `Hover.hpp` | `applyHover()` | Repaints a `DrawList` from a `HoverPointer`, deciding every target. |
| `WidgetId.hpp` | `WidgetId` | A caller-supplied symbolic id. |
| `ButtonSpec.hpp`, `ButtonState.hpp`, `ContainerSpec.hpp` | — | How a widget is described. |
| `Sizing.hpp`, `SizeMode`, `Axis.hpp`, `Alignment.hpp` | — | How space is asked for and shared. |
| `Theme.hpp` | `Theme` | One plain value per frame; there is no style stack and no cascade. |

`src/LayoutTree.hpp` and `src/Resolve.hpp` are private.

## Depends on

[`gfx`](gfx.md), and nothing else — not `event`, not `replay`, not `input`.

## Non-obvious decisions

**Immediate-mode calls build a flat arena; layout happens at `finish()`.**
A container cannot size itself from children it has not been told about yet, so a one-pass immediate-mode layout could not nest.
Deferring solves it: because a child is always appended after its parent, measuring is one descending index loop and arranging one ascending one.
Flat loops rather than recursion, so there is no nesting depth to exceed — and ascending order is also correct paint order for a renderer with no z-order.

**Mis-nesting is not expressible, with one case left over that is checked.**
`row()`/`column()`/`panel()` return a `Scope` that closes the container when it dies, and `Context` has no `end()` of any kind, so there is no unbalanced call to check for.
The exception is `finish()` called while a `Scope` is still alive, which a destructor cannot see coming: that one throws `ui::UiError` rather than laying out a half-built tree.

**The picture is a value.**
`Frame::commands` is a vector of plain `FillRect`/`DrawText` values, so a whole layout is asserted with `EXPECT_EQ` and no mock.
`paint()` never clears and never presents, since a UI is drawn over what is already there.

**A button is clickable, and the library still reads no device.**
The pointer arrives as a `ui::Pointer` argument to `Context`, defaulting to no pointer at all so a display-only caller is unchanged, an application having folded [`input`](input.md)'s edges into it.
A widget named with a `WidgetId` in its `ButtonSpec` works out its own hovered/pressed appearance, and `Frame::interactions` reports the `hovered`/`activated` id and whether the pointer is over anything the UI filled in.
`ButtonState` is still accepted as an override, for the app that knows which button is in play.

**Nothing is retained between frames.**
Interaction is resolved by a private `detail::resolve()` stage (`src/Resolve.hpp`) between `layout()` and `flatten()`, hit-testing the arena by *descending* index — ascending is paint order, so descending is front-to-back, and layout's containment guarantee makes the frontmost hit the deepest — and then writing each interactive node's background.
Everything is therefore resolved against the same frame's layout.
Activation is on the press, deliberately: a press-then-release match would be cross-frame state a replay would have to regenerate.
There is no pointer capture, no double-click and no hover delay, because the library reads no clock and no device.

**Ids are symbolic, not positional.**
A `WidgetId` is supplied by the caller rather than derived from declaration order, because that id is what crosses back into application state.

**The keyboard is an argument too, and focus is passed through rather than kept.**
Key edges arrive as a `ui::Keyboard` value — symbolic `ui::Key` values in arrival order, defined by this library rather than by any framework — defaulting to none, so an existing caller's output is byte-identical.

Focus is the one thing a keyboard UI needs that outlives a frame, and it does not live here: last frame's focused id goes *in* as a `Context` argument and this frame's comes back *out* as `Interactions::focused`.
So the state sits in application state, where a replay regenerates it from the recorded key presses, and the library stays as stateless as press-time activation requires.

The tab order is the arena's ascending index, which is declaration order, so no second order can drift from the layout.
A repeated id is one stop, an unnamed button is none, Tab from nothing takes the first widget and Shift+Tab the last, and both wrap.
Once focus is in play, a pointer press moves focus to whatever it activated, so the ring and the keystrokes cannot end up on different widgets — and a caller using the pointer alone never gains a ring it did not ask for.

Enter reports through `Interactions::activated` exactly as a press does, so one code path handles both.
The focused widget draws `Theme::focusRing` (yellow), `Theme::focusRingThickness` pixels thick, as four `FillRect`s appended *after* every widget, since `IRenderer` has no stroke and a container declared later would otherwise paint over a ring drawn in place.

**A text field and a dropdown hold nothing of their own either.**
A field's characters and caret arrive in `TextFieldSpec`, a list's open/closed and selected state in `DropdownSpec`, and what happened comes back as `Interactions::edit` and `Interactions::chosen`.
The application owns all of it, so a replay regenerates it from the recorded input rather than from anything the UI remembered.

Typing arrives on the same `Keyboard` the focus keys do rather than as a second input channel, and `TextFieldSpec::focused` is an override on top of the focus the `Context` was handed, so Tab reaches a field and Enter submits the one it landed on.

An open dropdown's list is an *overlay*: out of its parent's flow, hung beneath the box it dropped from, painted after every other command and hit-tested before them — which is the only way to be on top when `gfx` offers no depth but paint order.

**Where a widget ended up is a third answer off the same layout.**
`Frame::rects` reports one rectangle per distinct id the frame named, and `find(id)` answers nothing for an id this frame did not declare.
It exists so an application drawing its own art around a UI places that art *from* the layout rather than beside it: two independently computed layouts agree only until either one changes, which is precisely how [poker](../apps/poker.md)'s card art and its labels came to disagree.

`ContainerSpec` therefore carries an id as well, so a row or a panel can be named — which also makes it something the pointer reports as hovered or activated, since that is the one thing an id means here, and a child sits at a higher index so it still wins the hit-test against the container holding it.
Every named node answers rather than only containers, because a node carries one id whatever kind it is and a button's rectangle is as useful to something drawing behind it as a row's is.
The mapping is collected *inside* the arranging pass rather than by a pass of its own, so the rectangle reported is the one `flatten()` drew from by construction — including under the proportional shrink a cramped container applies, which is exactly where a repeated sum would diverge.
A repeated id keeps its last declaration, following the existing rule that two nodes sharing an id are one widget.

A caller that names nothing pays one integer comparison per node and a vector that never allocates.
Reading a rect back is safe anywhere, including inside the tick path, because a layout is a pure function of the declarations, the theme and the canvas — which is what makes it unlike [`input`](input.md)'s `PointerHintChannel`.

**Hover is a fourth answer, and it is the one that leaves the tick path.**
`Frame::hoverTargets` is one `HoverTarget` per named widget that works its own appearance out, and `applyHover(commands, targets, hover)` repaints the picture from a `HoverPointer`.
Hover and other pointer effects are **by definition not part of the simulation**, so they may decide what is drawn and nothing else, and three decisions make that structural rather than a promise.
`HoverPointer` carries a position and *no* `down` and *no* `pressed`, so it cannot say a press happened; `applyHover()` is handed a `DrawList &` and two read-only values and never a `Frame`, so `Interactions` is not reachable from inside it at any cost; and the position it draws from is a value cell ([`input`](input.md)'s `PointerHintChannel`) rather than a marked event, for the reason that channel already gives at length.

Activation, focus, edits and chosen options keep resolving in `detail::resolve()` from the recorded `Pointer` inside the tick path, exactly as before, and `ContextHoverTest` pins that a frame with all four in play has identical `Interactions` before and after a hover pass.

`applyHover()` decides **every** target rather than only the one under the pointer, because a gated stream leaves `Interactions::hovered` naming whatever the last press passed over, and lighting one up without putting the others out would leave that widget lit for the rest of the session.
A **held** target is stepped over, since a press is recorded input and its appearance is the simulation's answer rather than a hint's — `HoverTarget::held` is written by `resolve()` and is what keeps a button looking pressed while it is pressed.
Called with no position it changes nothing, so a caller that never opts in draws byte for byte the picture `finish()` produced.

Targets are collected inside `flatten()` for the reason rectangles are collected inside the arranging pass: a target names a command by index, so the only place that index is certain is the loop that appends it.

**There is no clipping, so containment is the layout's job.**
`IRenderer` has no scissor, so a container with too little room shrinks its children in proportion rather than letting them escape.

## Rules for callers

An application must describe and resolve its UI **inside the tick path, downstream of the recorder** — never in a renderer — so a replay stores the click and regenerates which widget it activated.
No `ui.*` event name may ever exist.
The canvas it is laid out and hit-tested against must be the *configured* window size, never the size a window reports, because a hit-test is a function of the layout and the layout is a function of the canvas.
[`apps/game`](../apps/game.md)'s `UiSink`/`UiOverlay` is the worked example, and [`apps/gfx_demo`](../apps/gfx_demo.md) is the showcase.

A hover pass is the exception to none of that: it runs on the render side, after the sink has resolved the press, and `apps/gfx_demo` is the worked example.
`game::UiSink`/`UiOverlay`/`Toolbar` should adopt it next — that app already owns a hint channel and already draws its placement ghost from it, so its toolbar buttons lighting up on approach is `main.cpp` handing `RenderSystem` the channel and one `applyHover()` call after the press has been resolved.
[`docs/hover-is-not-simulation.md`](../../docs/hover-is-not-simulation.md) is the rule written down.
