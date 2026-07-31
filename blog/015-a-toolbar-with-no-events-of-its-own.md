# A toolbar with no events of its own

*Post 15*

`apps/game` has a toolbar now: zoom out, zoom in, reset view, and a label reading the camera's zoom level back out.
It is four widgets and about forty lines, and it took longer to place in the architecture than to write.

[Post 13](013-the-camera-is-simulation-state.md) ended with a rule: **translation from input to meaning happens downstream of the recorder.**
A click is the input; the placement is regenerated.
A toolbar is the same rule again, with a UI library in the middle of it, and that middle is where it gets interesting.

## The event that must not exist, again

The obvious design writes itself.
The pointer is a device thing, so read it near the window; a button was clicked, so dispatch `ui.button_activated` with the widget's name in it; a sink zooms the camera.

Both halves are wrong, and each is wrong for its own reason.

`TickEventRecorder` sees every dispatched event.
A `ui.button_activated` would land in the `--record` file *alongside* the `input.pointer_down` that produced it, and replaying that file would zoom twice for one press — once from the recorded activation, once from the recorded click resolving against the bar again.
This is the third time this project has met that trap, and it now has a name in the code: no `ui.*` event name may ever exist.

The other half is worse because it is quieter.
Resolving the click in the renderer means resolving it against whatever the renderer happened to be drawing, which is not something a replay reproduces.
`antwika::gfx` is a write-only projection precisely so that nothing downstream of the tick can feed back into it, and a hit-test in the renderer is exactly that feedback.

So the toolbar is described and resolved **inside the tick path**, by an `ITickEventSink`, from the same recorded input everything else is folded from.
What a recording holds is a press at a pixel.
Which button that press hit is worked out again, every run, by the same code that worked it out the first time.

## The order the sinks run in is the design

`apps/game`'s sink list is short, and every position in it is load-bearing:

```
InputFold   ->   GameStateReducer   ->   UiSink   ->   GridSink   ->   StopSignal   ->   recorder
```

`InputFold` is first because it holds the decoded event the sinks after it are given, and it is the only thing that clears an edge — so the tick boundary is one rule in one place.

`UiSink` is before `GridSink` for two independent reasons, and it is nice when those coincide.
A press has to be resolved against the bar before the grid sees it, or a click on "zoom in" also lays a path tile under the button.
And `GridSink` is what runs the scheduler on `engine.tick`, so anything that must appear in this frame has to be folded ahead of it.

The recorder is last, which is the invariant the whole arrangement rests on: it records the input, and everything to its left is a consequence of that input rather than a peer of it.

## The one fact three collaborators share

Three things need to agree about the toolbar, and none of them should have to know what the others are.
`UiSink` writes the picture once per tick.
`RenderSystem` paints it over the grid.
`GridSink` asks whether a click was the bar's before treating it as the world's.

`UiOverlay` is that agreement, and it is deliberately a small shared state object rather than one collaborator asking another — the same shape `life::DragState` already had.
The renderer never learns what a pointer is; the grid never learns what a button is.

```cpp
void set(DrawList picture, bool covered);
[[nodiscard]] const DrawList &commands() const noexcept;
[[nodiscard]] bool pointerOverUi() const noexcept;
```

It holds nothing of its own.
What goes in is described from the recorded input and the simulation state, so a replay rebuilds the same picture and the same answer.

`RenderSystem` then paints it last, after the grid, which is how it reads as being in front:

```cpp
antwika::ui::paint(renderer, overlay.commands());
renderer.present();
```

`ui::paint()` never clears and never presents, because a UI is drawn over something.
That was decided when the library had no pointer at all, and it is what makes "draw the bar over the grid" one line rather than a layering feature.

## What the bar covers, it covers from the grid

A UI drawn over a world has to take clicks away from it, or every button press also builds a road.
`GridSink`'s first act is therefore to ask:

```cpp
// Whatever the toolbar covers, it covers from the grid too.
// A movement is exempt, so a pan begun on the grid can cross it.
if (overlay.pointerOverUi()
    && !std::holds_alternative<PointerMoved>(event))
{
    return;
}
```

The exemption for movement is the part worth arguing about.
Presses and scrolls are refused, so you cannot lay a tile through the bar and cannot zoom the world by scrolling over it.
Movement is not, so a middle-drag pan begun on the grid carries on when the pointer crosses the bar and does not stall halfway.

That asymmetry is defensible because of *what* a movement does here: it only pans while the middle button is already held, which means a press already established the gesture on the grid.
A movement can never start something the bar should have caught.

`pointerOverUi` comes from the UI library rather than the app, and its definition is narrower than "inside the bar's bounding box": it is true when the pointer is over something the UI *filled in*.
A node that draws nothing covers nothing, so a growing spacer in a transparent row is not a wall.

## The canvas is not the window

`Toolbar::describe()` takes a canvas, a pointer and a camera, and is a pure function of the three.
The canvas it is handed is `kUiCanvas` — the size the window was *asked* for — and never the size a window reports.

This is `life::PointerToggleSink`'s rule about cells, one level up: a hit-test is a function of the layout, and the layout is a function of the canvas.
Resolve a recorded click against a differently sized window and it resolves to a different button, or to no button at all, and nothing anywhere says so.
The file still parses, the run still completes, only the outcome differs.

`UiOverlay` owns that canvas rather than whoever describes the UI, so nothing can lay the bar out against one size and hit-test it against another.
And the constant lives in its own header rather than in `main.cpp`, because the app is not its only reader: a test that exercises the shipped wiring has to resolve a click against the same canvas the binary does, and a second literal is a second answer waiting to disagree.

Its doc comment carries the warning that follows: changing that number invalidates every existing recording.

## Describing it twice, on purpose

`UiSink::refreshAndAct()` calls `Toolbar::describe()` and then, sometimes, calls it again:

```cpp
// The zoom the bar reports has just changed.
// So it is described once more.
// Otherwise it would show the level it was pressed at.
if (activated != kNoWidget)
{
    frame = toolbar.describe(
        overlay.canvas(), pointerNow(pressed), camera);
}
```

This is the price of a UI library that retains nothing, and it is the price the library told you to pay: activation is known only once the frame's picture has been decided, so acting on it cannot change that picture.
To show the result in the same tick, describe the UI again after acting and keep the second frame.

It looks like waste and it is not.
A frame is a flat arena and a vector of `FillRect`/`DrawText` values, built from four widget declarations, twice, on the ticks where a button was actually pressed.
The alternative is a library that remembers something between frames, and the whole of [post 14](014-a-button-that-remembers-nothing.md) is about why that costs more.

The sink also describes the bar on plain `engine.tick`, with no press, for the renderer that is about to paint — so what gets painted shows the state the tick ends with rather than the state it started in.

## No toolbar means no toolbar

`bootstrap()` takes the overlay as an optional, and registers `UiSink` only when there is one.

The first version registered the sink unconditionally and gave it a default-constructed overlay when nobody supplied one.
That is a zero-sized canvas, which lays the bar out into nothing, which no click can hit — so nothing is ever hovered or activated and it *behaves* like having no toolbar.

Behaving like it is not the same as being it.
An unhittable toolbar is a toolbar whose absence depends on arithmetic about an empty rectangle, and that is a fact somebody has to keep true.
Not registering the sink makes "no toolbar" mean no toolbar, and the grid still gets an overlay it can ask — one nothing writes to, which therefore covers nothing, so every click is the world's.

That is the whole trick in this app, stated one more time: the toolbar changes what a click *means*, never what a recording *holds*.
