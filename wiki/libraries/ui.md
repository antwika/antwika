# antwika::ui

`src/libs/ui/` — immediate-mode UI whose output is a value.

## What it is for

Describing a nestable layout of rows, columns, panels, labels and buttons, and turning it into a list of drawing commands plus the interactions the pointer produced.

## Key types

| Header | Type | Role |
| --- | --- | --- |
| `Context.hpp` | `Context` | The immediate-mode surface: `row()`, `column()`, `panel()`, `label()`, `button()`, `spacer()`, `finish()`. |
| `Scope.hpp` | `Scope` | A `[[nodiscard]]` guard returned by every container call, closing it in its destructor. |
| `Frame.hpp` | `Frame` | What `finish()` returns: `commands` (a `DrawList`) plus `interactions`. |
| `DrawList.hpp`, `DrawCommand.hpp` | `DrawList`, `FillRect`, `DrawText` | The picture, as plain comparable values. |
| `Painter.hpp` | `paint()` | The only thing in the library that touches an `IRenderer`. |
| `Pointer.hpp` | `Pointer` | The pointer, passed in as an argument; the default is no pointer at all. |
| `Interactions.hpp` | `Interactions` | The `hovered` and `activated` `WidgetId`, and whether the pointer is over anything the UI filled in. |
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

**Mis-nesting is not expressible.**
`row()`/`column()`/`panel()` return a `Scope` that closes the container when it dies, and `Context` has no `end()` of any kind, so there is no unbalanced call to check for.

**The picture is a value.**
`Frame::commands` is a vector of plain `FillRect`/`DrawText` values, so a whole layout is asserted with `EXPECT_EQ` and no mock.
`paint()` never clears and never presents, since a UI is drawn over what is already there.

**Nothing is retained between frames.**
Interaction is resolved by a private `detail::resolve()` stage between layout and flatten, hit-testing the arena by *descending* index — front-to-back, and layout's containment guarantee makes the frontmost hit the deepest.
Activation is on the press, deliberately: a press-then-release match would be cross-frame state a replay would have to regenerate.
There is no pointer capture, no double-click and no hover delay, because the library reads no clock and no device.

**Ids are symbolic, not positional.**
A `WidgetId` is supplied by the caller rather than derived from declaration order, because that id is what crosses back into application state.

**There is no clipping, so containment is the layout's job.**
`IRenderer` has no scissor, so a container with too little room shrinks its children in proportion rather than letting them escape.

## Rules for callers

An application must describe and resolve its UI **inside the tick path, downstream of the recorder** — never in a renderer — so a replay stores the click and regenerates which widget it activated.
No `ui.*` event name may ever exist.
The canvas it is laid out and hit-tested against must be the *configured* window size, never the size a window reports, because a hit-test is a function of the layout and the layout is a function of the canvas.
[`apps/game`](../apps/game.md)'s `UiSink`/`UiOverlay` is the worked example, and [`apps/gfx_demo`](../apps/gfx_demo.md) is the showcase.
