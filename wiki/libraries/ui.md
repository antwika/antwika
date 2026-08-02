# antwika::ui

`src/libs/ui/` — immediate-mode UI whose output is a value.

## What it is for

Describing a nestable layout of rows, columns, panels, labels and buttons, and turning it into a list of drawing commands plus the interactions the pointer produced.
Everything is laid out arithmetically from `gfx::textSize()` alone, so the library asks no backend to measure anything, and it is drawn through [`gfx`](gfx.md)'s rectangle and text calls.

## Key types

| Header | Type | Role |
| --- | --- | --- |
| `Context.hpp` | `Context` | The immediate-mode surface: `row()`, `column()`, `panel()`, `label()`, `button()`, `textField()`, `textArea()`, `dropdown()`, `spacer()`, `finish()`. |
| `Scope.hpp` | `Scope` | A `[[nodiscard]]` guard returned by every container call, closing it in its destructor. |
| `Frame.hpp` | `Frame` | What `finish()` returns: `commands` (a `DrawList`), `interactions`, `rects` and `hoverTargets`. |
| `DrawList.hpp`, `DrawCommand.hpp` | `DrawList`, `FillRect`, `DrawText` | The picture, as plain comparable values. |
| `Painter.hpp` | `paint()` | The only thing in the library that touches an `IRenderer`. |
| `Pointer.hpp` | `Pointer` | The pointer, passed in as an argument; the default is no pointer at all. |
| `Interactions.hpp` | `Interactions` | The `hovered`, `activated` and `focused` `WidgetId`, the `edit`, `chosen` and `scrolled` results, and whether the pointer is over anything the UI filled in. |
| `ScrollChange.hpp` | `ScrollChange` | Which line a text area is actually showing at its top, when that is not the one asked for. |
| `Keyboard.hpp` | `Keyboard`, `Key` | Key edges in arrival order, plus a `typed` view of the characters each `Key::Character` edge takes one of; defaults to none. |
| `TextFieldSpec.hpp` | `TextFieldSpec`, `TextEdit` | A field's characters, caret and focus going in; what happened coming out. |
| `TextAreaSpec.hpp` | `TextAreaSpec` | The same, over many lines: a document, one flat caret index into it, the selection's far end, which line is at the top, and the room it is given. |
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

Enter reports through `Interactions::activated` exactly as a press does, so one code path handles both — and when the focused widget is a dropdown option it fills in `Interactions::chosen` too, from the owner and index the arena already carries beside the option's id.
That is what makes the claim true rather than nearly true: reporting only the id would leave every caller subtracting `DropdownSpec::optionIdBase` back out of it, and an option left unnamed could never be chosen by keyboard at all.
The focused widget draws `Theme::focusRing` (yellow), `Theme::focusRingThickness` pixels thick, as four `FillRect`s appended *after* every widget, since `IRenderer` has no stroke and a container declared later would otherwise paint over a ring drawn in place.

**A text field and a dropdown hold nothing of their own either.**
A field's characters and caret arrive in `TextFieldSpec`, a list's open/closed and selected state in `DropdownSpec`, and what happened comes back as `Interactions::edit` and `Interactions::chosen`.
The application owns all of it, so a replay regenerates it from the recorded input rather than from anything the UI remembered.

Typing arrives on the same `Keyboard` the focus keys do rather than as a second input channel, and `TextFieldSpec::focused` is an override on top of the focus the `Context` was handed, so Tab reaches a field and Enter submits the one it landed on.

**A character is an edge in that list too**, taken by a `Key::Character` edge indexing into `Keyboard::typed`.
It reads as indirection until you type `a`, Backspace, `b` inside one frame: the characters used to go in as a lump before any key was read, so what came out was `a` rather than `b`, and a caller that folds a whole tick's typing into one frame hit that at ordinary typing speed.
A character with no edge to take it is not typed at all, since nothing would say where in the order it belonged.

**`textArea()` is `textField()` over many lines**, and the whole of the difference is what the keyboard means: Enter writes a line break rather than submitting, `Key::MoveUp` and `Key::MoveDown` walk the caret between lines keeping its column where the line beside it is long enough, `Key::MoveLineStart` and `Key::MoveLineEnd` put it at the caret's own line's two ends, and the box takes the room it is given rather than one line's worth.
The caret is one flat index into the document rather than a row and a column, so an application storing a `std::string` and a `std::size_t` is storing everything a replay has to regenerate -- and a line break is just a character in the text, which is what makes that true.
A blank line is drawn as a row opened over a strut a glyph cell tall, because an empty text node measures nothing at all and the lines below it would otherwise move up.
[music_editor](../apps/music_editor.md) is what it was written for.

**A selection is a second index and nothing else.**
`TextAreaSpec::anchor` is the far end, the characters between it and the caret are drawn on `Theme::selection`, and every key that writes -- a character, Enter, Backspace, `Key::Delete`, `Key::Cut` -- takes the whole of it first.
`TextAreaSpec::highlights` is the same picture with none of the consequences: caller-marked spans drawn on `Theme::highlight` -- a live-coding editor lights the notes that are sounding with it -- that move no caret, join no selection and lose to the selection's ground wherever the two overlap.
`Key::SelectLeft`, `SelectRight`, `SelectUp`, `SelectDown`, `SelectLineStart` and `SelectLineEnd` are separate keys rather than a shift flag beside the six moves, for the reason `FocusPrevious` is a key rather than a flag on `FocusNext`: a modifier is held state and everything crossing this seam is an edge.
The anchor is a `std::optional`, absent meaning "wherever the caret is", because every index is a place a selection can really end -- including the end of the text, which a sentinel would have taken.

**A copy is reported and a paste is typed**, which is the only shape a clipboard can have here.
`Key::Copy` and `Key::Cut` put the selected characters in `TextEdit::copied` and this library forgets them immediately, since it retains nothing; where they go is the application's, and [music_editor](../apps/music_editor.md) keeps them in its own state and mirrors them outward to the window system's clipboard from there, while a paste reaches it as a recorded event -- so a replay pastes what the run pasted rather than whatever the replaying machine happens to hold.
Pasting therefore needs no key at all: the application puts the characters in `Keyboard::typed` with a `Key::Character` edge each, exactly as if they had been typed.

**A press inside an area puts the caret where it landed**, and that is the one answer a widget gives that needs the layout.
Which character a click is on is a function of where the area was arranged, so it is worked out in `resolve()` rather than where the area was declared, and it amends whatever the frame's keys already came to rather than replacing it.
The arithmetic is exact because `gfx::kGlyphAdvance` and `kGlyphLineHeight` are frozen: a line is a division, a column is a division, and a click below the last line is the end of the text.
`Pointer::extends` is what makes a press carry the selection on rather than start a new one -- a shift-click, or a drag -- and it is on the pointer rather than in the keyboard because it is a property of that press edge.

**An area scrolls in whole lines, and says which one it is showing.**
`TextAreaSpec::scroll` is the line at the top and `Interactions::scrolled` is the line actually drawn there, reported only when the two differ, so a caller that stores the answer and hands it back settles after one frame.
Three things move it: a press or drag on the bar `TextAreaSpec::scrollbar` asks for, a caret that has walked out of view -- followed only on a frame that reported an edit, so a bar drag is not pulled straight back to it -- and a requested line so far down there is nothing left to show.
It is whole lines rather than pixels for the reason the zoom in [game](../apps/game.md) is an index into a table of whole sizes: what a recorded click resolves against has to be something a replay reaches exactly.

That needed one thing of the layout: `Node::clips`, set on the column holding an area's lines and nowhere else.
A container with more asked of it than it has room for cuts every child down in proportion, which for a document longer than its pane is a page of lines too short to draw a glyph in -- a blank pane.
A clipping container keeps its children's own sizes and lets the placement that already keeps a child inside its parent clamp the ones past the bottom edge to nothing, and it asks its own parent for nothing on their behalf, so a pane is as tall as it was given rather than as tall as what it holds.

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
A target sharing the frontmost one's id lights up with it, since two nodes carrying one id are one widget here as everywhere else — deciding this route on geometry alone would light half of such a widget and `resolve()`'s dressing all of it.
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
