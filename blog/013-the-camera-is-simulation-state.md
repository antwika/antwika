# The camera is simulation state

*Post 13*

`apps/game` was the smallest thing in the repository: a struct with a score in it, a reducer that added numbers to it, and a `main` that printed the total.
It is now an isometric grid you build on with the mouse.
Left-click lays a path tile, right-click drops a walker onto one, middle-drag pans, the wheel zooms, and the walkers set off along the paths preferring a right turn at every junction.

This post is about the one decision that decided everything else, and it is not "how do I draw a diamond".

## The bug that would have shipped

Panning and zooming change nothing about the simulation.
A walker goes where the paths take it whether you are looking at the grid from close up or far away, and nothing in the rules mentions a viewport.
So the camera belongs to the renderer.
That is the obvious answer, it is what every instinct says, and it is wrong in a way no test in this repository would have caught.

A click does not arrive as a cell.
It arrives as a pixel — `input.pointer_down` at (412, 118) — and turning that into "cell (5, 2)" needs to know where the camera was pointing and how far it was zoomed in.

Put the camera in the renderer, and a replay resolves recorded clicks against whatever camera the replay happens to have.
The replay is still perfectly deterministic.
It just deterministically puts the paths somewhere else, and `--record` output becomes silently useless.

So the camera is folded from replayable input by a tick sink, exactly like the score is, and the renderer only ever reads it.
Everything awkward below follows from that one sentence.

## Consequence one: no floating point

`screenToCell` feeds state, so it has to give the same answer under GNU, LLVM and MinGW, and the same answer during a `--record` run as during its `--replay`.

A float scale factor would almost certainly do that.
Integers do it provably, so zoom is an index into a table of whole tile half-widths:

```cpp
inline constexpr std::array<std::uint32_t, 5> kZoomHalfWidths{4, 8, 16, 32, 64};
```

Zooming steps that index and clamps.
Every projection is then an integer multiply or divide, and no `double` appears in any header the app added.

The inverse is where the interesting bug lives.
Given the forward projection

```
screen.x = (cell.x - cell.y) * hw + pan.x
screen.y = (cell.x + cell.y) * hh + pan.y
```

the obvious inverse divides by `hw` and `hh` and rounds twice.
Multiplying through first and dividing once at the end is exact:

```cpp
cell.x = floorDiv(u * hh + v * hw, 2 * hw * hh);
cell.y = floorDiv(v * hw - u * hh, 2 * hw * hh);
```

And `floorDiv` is not `operator/`.
C++ integer division truncates toward zero, so `-1 / 4` is `0` where the floor is `-1`.
The grid reaches negative screen coordinates the moment the camera pans, and truncation there makes the cells straddling each axis twice as wide as every other cell.
That is a bug you cannot see until somebody pans past the origin, which is to say not in the first test anybody writes.
There is a test for it now, and it is three lines.

## Consequence two: the window's size is not allowed near it

Centring the grid on the canvas is the natural thing to do.
It also puts the window's size into `screenToCell`, and therefore makes a window resize change which cell a pixel means — which would make a resize *replayable input*, with `gfx::Resized` having to travel the same road as `CloseRequested`.

Anchoring the projection to the camera's pan instead costs one constant in `main` and buys the whole problem going away.
The window's size then reaches nothing but the culling test and the background fill, which is what `apps/life`'s `RenderSystem` has always claimed for itself and why that claim is true there too.

This also settles how zooming is anchored.
Zoom has to be anchored to *something*, and the canvas centre is exactly what we just banned.
So it anchors to the cell under the cursor — the only anchor available that does not need to know how big the window is.

Getting that right took two attempts.
The first version snapped the pan so the anchor cell's corner sat under the cursor, which moved the camera even when zooming by zero notches.
The test that caught it was `ZoomedAt_ReturnsTheCameraUnchangedForNoNotches`, written because it seemed too obvious to be worth writing.

## The event that must not exist

Here is the rule that keeps the whole thing honest, and it is a rule about what *not* to add.

`apps/game` defines no event for placing a path.

The temptation is obvious: a click happens, so dispatch `game.place_path` with the cell in it, and let a reducer handle that.
But `TickEventRecorder` sees every dispatched event.
It would write `game.place_path` into the `--record` file *alongside* the `input.pointer_down` that caused it, and replaying that file would lay two tiles for one click.

This is the `engine.tick` trap from [post 12](012-a-window-that-cant-talk-back.md) wearing a different hat, and it is the third time this project has met it.
The rule that falls out is: **translation from input to meaning happens downstream of the recorder.**
A click is the input; the placement is regenerated.
`GridSink` sits in the tick path and does that translation, and the replay stores only the click.

There is a test asserting no event named `game.place*` ever appears in a recording, so adding one later fails loudly rather than silently doubling every click.

## Two rules that turned out to be one

Walkers "prefer to turn right at an intersection" and "reverse at a dead end".
That reads like two rules with a branch between them.
It is one preference order:

```cpp
right, straight on, left, back
```

Take the first that has a path.
"Prefers right" is the first entry.
"Reverses at a dead end" is `back` being *last*: a walker turns round only when nothing else is available, which is the definition of a dead end.
No branch tests for one.

The whole rule is a pure function of a facing and four booleans, so there are 64 possible inputs and the test checks all 64 rather than sampling.
When the rule *is* the feature, exhaustive is cheap.

One case had to be invented rather than derived: a walker dropped on a one-tile path has nowhere to go, including backwards.
`nextFacing` returns `nullopt` and the walker stays put, because a rule forced to return something would have to make a move up.

## Drawing needed a new primitive, and it earned it

`IRenderer` could clear, fill rectangles and draw text.
An isometric diamond has no axis-aligned edges.

The cheap option was to fill diamonds out of horizontal `drawRect` calls in app code and leave `antwika::gfx` alone.
The seam-widening option was to add `drawLine`.
The seam won, because a lattice is the thing this app draws most of and a lattice is lines.

`drawLine`'s contract has one clause worth the trouble: **both endpoints are included**, so a line from a point to itself draws that pixel.
That sounds like pedantry until you see how a diamond gets filled — one horizontal line per row, tapering to nothing at the top and bottom corners.
Those end rows are zero-length lines.
A backend that dropped an endpoint would leave every diamond missing its points.

raylib does exactly that: `DrawLine` submits a two-vertex GL primitive, which covers no pixel when both vertices coincide.
So the raylib backend special-cases it to `DrawPixel`, and the conformance suite has a case that would have caught it.

## The queue that two seams had to share

`antwika::input` deliberately does not depend on `antwika::gfx` — reading a key should not require opening a window.
Two independent seams, no coupling.

Except SDL has one process-global event queue, and `SDL_PollEvent` drains it.
Two backends polling it independently starve each other, non-deterministically, depending on which polls first.

The fix is not a rule the two libraries cooperate on.
It is `Sdl3Pump`, inside `backends/sdl3/`, which drains SDL once and routes each event into a window queue or an input queue, so whichever subsystem polls first advances the other too.
`SDL_Init` and `SDL_Quit` moved there with it, reference-counted, so SDL starts with the first backend of either kind and stops with the last.

The framework's single queue is a fact about the framework.
It gets admitted in the framework's own directory, behind both abstractions, rather than leaking upward as something `src/` has to know.

raylib has the opposite problem: no queue at all, only global state.
So `RaylibInputBackend` diffs that state against what it last reported and synthesises the edges — which is precisely what the input conformance suite's "stays drained when polled again" case exists to catch, and the reason that case was written before either real backend existed.

## What it cost, and what it bought

The bill: a new primitive on the graphics seam, an input library finished off, three copies of `TickPacer` in this repository now instead of two.
That last one is flagged in the class's own doc comment along with where it belongs instead, because a duplication nobody wrote down is a duplication nobody fixes.

What it bought is one line of terminal output:

```
$ xvfb-run -a build-sdl3/bin/antwika_game --record session.replay
$ build/bin/antwika_game --replay session.replay
```

Those two runs print identical state, across two different graphics backends and two different input backends, from a recording that contains twelve clicks, two scroll notches and a stop — and nothing else.

The camera being simulation state is what makes the second line mean anything.
