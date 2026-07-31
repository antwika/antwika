# A button that remembers nothing

*Post 14*

`antwika::ui` shipped display-only, and said so out loud: a button was *told* how it should look, and the library read no pointer, no keyboard, nothing outside its arguments.
`REQUIREMENTS.md` carried it as a Won't-have.
That was honest at the time — `antwika::input` was vocabulary with no backend behind it, so there was no build in which a button could have been clicked.

It is clickable now, and the library still reads no device.
This post is about the one property that decided the design of everything else: **nothing is retained between frames.**

## The idiom that could not work here

Every immediate-mode UI anybody has used returns the click from the call that declares the widget:

```cpp
if (ui.button("ok")) { … }
```

That cannot work in this library, and the reason is not interaction at all — it is the deferral that made nesting work in the first place.
At the moment `button()` is called, nothing knows where the button will end up.
The call appends a node to a flat arena; the arena is measured and arranged only when `finish()` is called, because a container cannot size itself from children it has not seen yet.
A `button()` that answered "were you clicked" would be answering about a rectangle that does not exist.

There is a standard workaround: hit-test this frame's click against *last* frame's rectangle for the same widget.
It is worth writing down why it was rejected, so it does not read later as something nobody thought of.

It makes the library retain a whole frame's layout between frames, which is exactly the property this design does not have.
It resolves a click against a canvas that is not the one the click will be drawn against, so a resize or a conditional layout silently mis-attributes the first click after it.
And it makes widget identity load-bearing *across* frames, so a layout declaring a different set of widgets this frame quietly re-points last frame's rectangles at them.

So the caller names the widget when it declares it, and asks afterwards:

```cpp
const auto frame = ui.finish();

if (frame.interactions.activated == kZoomIn)
{
    camera.zoomIn();
}
```

One canvas, one layout, one pointer position, all from the same frame.

## Activation is on the press, and that is a replay decision

Almost every toolkit fires a button on press-then-release-over-the-same-widget.
That requires remembering, between frames, which widget captured the press.

In this repository, "remembering something a click is interpreted against" is a loaded phrase.
It is the whole lesson of [post 13](013-the-camera-is-simulation-state.md): anything a recorded click is resolved against has to be regenerable from replayed input, or the replay lands somewhere else.
Capture state is precisely that kind of thing — invisible, cross-frame, and owned by a library that the replay system knows nothing about.

So a button activates **on the press**:

```cpp
// Nothing hovered means there is nothing to activate.
// So a press over no widget copies kNoWidget, needing no guard.
if (pointer.pressed)
{
    interactions.activated = interactions.hovered;
}
```

With that, `finish()` is a pure function of the declarations, the canvas and this frame's pointer.
No capture, no cross-frame id stability requirement, nothing for a replay to regenerate beyond the pointer state the application is folding anyway.

The cost is real and it is not hidden: you cannot cancel a press by sliding off the button before letting go.
`ButtonState::Pressed` is still shown while the button is held over the widget, which is honest — it says "this is the one you are on", not "this is about to fire".
`game::GridSink` had already made the same trade for the same shape of reason, and its comment is the precedent: a left-drag pan would need a "moved more than N pixels, so that was a drag" rule, which moves placement to the release and invents a threshold nothing else here justifies.

## One new stage, and it is pure

The pipeline was three stages and a paint:

```
Context calls  ->  LayoutTree      flat arena of Node
+ canvas       ->  layout()        measure, then arrange
+ pointer      ->  resolve()       hit-test, then style     <- new
               ->  flatten()    -> DrawList (a value)
+ IRenderer    ->  paint()         write-only translation
```

`resolve()` is the entire behaviour change.
It runs *after* `layout()`, because a hit-test needs somewhere to hit, and *before* `flatten()`, because what it decides is a background colour that flattening then emits.
`layout()`, `flatten()` and `paint()` are untouched, and the drawing side stays exactly as write-only as it was.

The hit-test itself is four lines of loop, and the interesting part is the direction:

```cpp
for (std::size_t index = tree.size(); index-- > 0;)
```

Ascending index is paint order — that is what made the flat arena work in the first place, since a child is always appended after its parent.
So descending index is front-to-back, and layout's containment guarantee (a child never escapes its parent) makes the frontmost hit also the deepest one.
One loop answers both "what is on top" and "what is innermost", because in this arena they are the same question.

Containment doing that much work is why the library still has no clipping: `IRenderer` has no scissor, so a container with too little room shrinks its children in proportion rather than letting them spill.
That was a layout decision made before there was a pointer, and it is what makes the hit-test one loop instead of a tree walk with a clip stack.

## Two small things the hit-test had to get right

Rectangles are half-open:

```cpp
// Half-open, so two touching rectangles cannot both be hit.
// A collapsed one is therefore hit by nothing.
return x >= left && x < left + rect.size.width && y >= top
       && y < top + rect.size.height;
```

Half-open falls out of wanting two adjacent buttons to have no shared pixel, and the zero-size case falls out for free: a collapsed rectangle is hit by nothing, rather than by everything at its origin.

The arithmetic is 64-bit, and the comment says why:

```cpp
// A right edge is an int32 origin plus a uint32 extent.
// That is exactly the sum that wraps, hence 64 bits.
```

A signed origin plus an unsigned extent is the one addition in this library that can overflow, and a wrapped right edge means a widget that is hit from the far side of the canvas.

## The pointer is an argument, so the library stays a leaf

```cpp
Context(Size canvas, Theme theme, Pointer pointer = {});
```

`antwika::ui` gained no dependency for any of this.
It does not link `antwika::input`, does not name `InputEvent`, and never sees an edge.
An application folds edges into `input::InputState` — which is what that class is for — and hands across a plain value with a position, a `down` and a `pressed`.

Two things survive because of that.
The library stays a leaf on `antwika::gfx`, so a test drives a button by writing a `Pointer` literal, with no input library, no backend and no fake anywhere in sight.
And the requirement that no file under `src/libs/ui/` may read state outside its arguments keeps holding word for word, because the pointer *is* an argument.

The default argument does more than it looks.
`Pointer{}` reports no pointer at all, so every display-only caller compiles unchanged and draws exactly what it drew before.
`position` is `std::optional<Point>` rather than an origin, for the same reason: an origin is a real place a widget can be, and "nothing has reported a position yet" is not.

## Ids are chosen, not counted

`WidgetId` is a scoped enum over `std::uint64_t` with no enumerators, following `gfx::WindowId` and `ecs::Entity`.
The caller picks the values.

Declaration order would have been free, and it is wrong for one reason: an index shifts the moment a layout gains a conditional widget, and this is the value that crosses back into application state.
An id has to keep meaning the same widget when the layout around it changes.

Two widgets sharing an id is legal, and means they are one widget — the topmost is hovered and activated, both take the resolved appearance.
That is a useful behaviour and a plausible mistake at the same time, which is the sort of thing that normally gets a runtime check nobody sees.
Here the ids are constants, so it can be a build error instead:

```cpp
static_assert(
    assertDistinct(kZoomOut, kZoomIn, kResetView),
    "every toolbar widget needs its own id");
```

`assertDistinct` compares every pair rather than sorting, because a frame declares a handful of ids and the quadratic loop is the one a constant evaluator can run without a copy.

## What `finish()` cannot do for you

One consequence is sharp enough that it is in the doc comment on `Interactions::activated`.

A press is resolved while the frame is being laid out, so what was activated is known only once the picture beside it has been decided.
Whatever the caller changes in response is therefore *not* in that picture.
Press "zoom in" and the frame you are holding still shows the old zoom level.

The library's answer is not to retain anything.
It is: describe the UI again after acting, and draw the second frame.
`game::UiSink` does exactly that, and says so where it does it — the second `describe()` costs one more arena and buys a bar that reports the state the tick actually ends with.

That is the shape of every fix in this library.
Frames are cheap because they are values; state between them is expensive because it is invisible.
Given the choice, it builds another frame.
