# Resizable windows, and the size an application is allowed to believe

`antwika::gfx` can open a window the user is allowed to resize, and a window can be asked what size it currently is.
Neither of those is new machinery; what is new is that the two sizes now have separate names, so the rule below is expressible in code rather than only in a comment.

## The two sizes

`WindowDesc::size` is the **configured** size: a number the application chose and passed to `IGfxBackend::createWindow()`.
`IWindow::configuredSize()` reports it back unchanged, under every backend, for as long as the window object lives, and it keeps reporting it after `close()`.

`IWindow::size()` is the **reported** size: whatever the window system currently says the drawable area is.
On a resizable window it follows the user around as they drag an edge.
On a window that is not resizable it may *still* differ from the configured size, because a window manager is free to hand back something other than what was asked for, and because a high-DPI display can multiply it.

`WindowDesc::resizable` defaults to `false`, which is what every window in the repository already meant before the field was documented this way, so no existing application changed behaviour.

## The rule

**Simulation state and hit-testing are functions of the configured size, never of the reported size.**

The reported size is read-only information flowing outwards from the window system, and it must not flow back in.
This is the same constraint [`blog/012-a-window-that-cant-talk-back.md`](../blog/012-a-window-that-cant-talk-back.md) states for window lifetime, applied to geometry: rendering is a write-only projection of state, so anything the renderer learns from the window is not allowed to reach the tick loop.

The concrete failure it prevents is a replay that does not replay.
A recorded session holds a pointer press at a pixel, and which cell or which widget that press means is worked out inside the tick path from a layout.
If that layout were a function of the reported size, the same recording would resolve to a different cell on a machine whose window manager chose a different size, or on the same machine after somebody dragged an edge mid-session.
Making a resize *replayable input* — sending `gfx::Resized` down the same road as `CloseRequested` — is the only other way out, and it is a much larger commitment: every recording would then have to carry the window geometry, and a replay would have to force it.
Choosing the configured size costs nothing and needs no format change, so that is what is chosen here.

`life::PointerToggleSink` and `game::UiOverlay` are the worked examples on the application side: both lay out against the size the window was *asked* for, and `game::UiOverlay` exists precisely so that nothing can lay a toolbar out against one size and hit-test it against another.

## What the reported size *is* for

Placing what is drawn inside the drawable area, and nothing else.
Centring a fixed-size picture in a window the user has made bigger is the motivating case: the picture's own geometry stays a function of the configured size, and only the offset it is blitted at comes from the reported one.
An offset applied uniformly to every drawing call is safe by construction, because it is applied after every decision has already been made, and because it is never asked what a pixel means.

## An offset *and a uniform scale*, which is the same argument

That paragraph is now read one step further, and the step is deliberate: **a uniform scale applied to every drawing call is safe for exactly the reason a uniform offset is.**
It is applied after every decision has already been made, it is applied identically to everything, and it is never asked what a pixel means.
So a fixed-size canvas may be *enlarged* into a window the user made bigger as well as centred in one, and `apps/game` is the worked example -- the whole city, its toolbar and every hit test are a function of `game::kUiCanvas`, and the window's reported size decides only how big the result is drawn and where.

`gfx::Viewport` is that transform, and `gfx::viewportFor(reported, canvas)` works it out.
`gfx::ViewportRenderer` is an `IRenderer` decorator that puts every call through it, so no backend and nothing that draws learns the transform exists.
Two properties of it are load-bearing rather than incidental.

**The scale is a ratio of two integers, and every coordinate it produces is integer arithmetic.**
Rendering may use floating point freely, since nothing drawn is read back -- but the transform is also run *backwards*, and what comes back that way is recorded input.
A float whose last bit differed between two toolchains would cost a divergent session rather than a misplaced pixel, which is the argument `AtlasText.hpp` already makes about the font's metrics.

**The height is what drives the scale, and the aspect ratio is fixed.**
A window's width decides how much of it is bar, never how big the picture is, so a narrow monitor and a wide one of the same height draw the game equally tall.
The width caps the scale in the one case where honouring the height would push the canvas past the window's edges, because a toolbar drawn off screen is a toolbar nobody can click and no pointer mapping can give that back.
The remainder is left as pillarboxes or letterboxes, which is what keeps every anchor on the canvas -- an edge, a corner, a centre -- a function of the canvas.

**Widening the *canvas* with the window is still refused**, and this document is where that refusal is recorded.
A wide monitor showing more world sounds like the same idea and is the opposite of it: a UI anchored to the right edge would become a function of the reported size, a recorded click near it would resolve differently on another machine, and the only way out would be the one rejected above -- making a resize replayable input, so that every recording carried the window geometry and every replay forced it.

## The pointer, and where the inverse belongs

A picture that is scaled needs a pointer that is scaled back, and **where that happens is the whole of what makes it safe**.

It happens *upstream of the recorder*, so what lands in a recording is already a canvas coordinate.
`input::IPointerMapping` is the seam, `input::MappedPointerSource` is the decorator that rewrites every positional edge, and `app::WindowPointerMapping` is the implementation that runs `gfx::viewportFor()` backwards -- in `antwika::app` because nothing lower may name a window and a pointer in one sentence.
`input::InputPipeline` attaches it immediately outside `LiveInputSource` and inside `PointerHintSource`, so the hint channel and the tick stream are in the same coordinates.

It is attached **only when a device is being read**, which is the one asymmetry in that stack.
A file already holds canvas coordinates; mapping them again on a replay would map them twice.

The consequence is the property the whole rule exists to protect, now stated positively rather than as a prohibition: a session recorded on a window of one size replays on a window of any other, on any machine, with no window geometry in the file and nothing in the tick path aware that a window has a size at all.
`src/apps/game/tests/ViewportReplayTest.cpp` is where that is asserted end to end.

A scale that is not a whole number maps a canvas pixel to a window pixel and back to within one pixel rather than onto itself.
That costs a replay nothing -- the file is what is replayed, and the file holds whichever canvas pixel the pointer was over -- and it is why the assertion above is about the replayed *result* rather than about the arithmetic round-tripping.

## Fullscreen is the same rule again

`WindowDesc::fullscreen`, `IWindow::isFullscreen()` and `IWindow::setFullscreen()` are `resizable`'s counterparts, and they exist on the same terms: **going fullscreen changes what `size()` reports and changes nothing else.**
It never touches `configuredSize()`, so with the scaling above in place, filling the screen enlarges the picture and moves no hit target.

**A fullscreen toggle is an action on the window, not simulation state**, so it may not live in a sink: a sink is downstream of the recorder and inside the tick path, where everything is a function of state a replay reproduces.
`app::FullscreenToggleSource` is where it lives instead -- a pure observer of the event stream, above the loop, which reads a key press and calls `setFullscreen()`.
The key press itself is ordinary recorded input, so a replay of a session in which somebody pressed it fills the screen at the same tick, and reaches the same state either way.
Which of the two happened is not something a run can tell, and that is the property worth having.

It holds an `IWindow &` rather than a `WindowId`, unlike `simulation::WindowInputSource`, and the difference is deliberate: that class holds an id precisely so it cannot close a window a renderer is still drawing into (see [`blog/012`](../blog/012-a-window-that-cant-talk-back.md)), and nothing here can close anything.

## Per backend

`null` accepts `resizable` and honours it by having nothing act on it: with no window system present, `size()` and `configuredSize()` are equal forever.
That is the deliberate headless behaviour, not an omission — a headless run must be the one where the reported size cannot surprise anybody.
It accepts `fullscreen` on exactly those terms: `isFullscreen()` reports whatever was last asked for, and neither size moves.
That is also why the viewport above costs a headless run nothing — `viewportFor()` of two equal sizes is the identity, in lowest terms, so every drawing call and every pointer position goes through unchanged and the surround is four empty rectangles nobody draws.

`sdl3` passes `SDL_WINDOW_RESIZABLE` when asked, reads the live drawable size through `SDL_GetWindowSizeInPixels()` on every `size()` call, and emits `gfx::Resized` from `SDL_EVENT_WINDOW_RESIZED`.
It latches the last size it saw at `close()`, so a closed window still answers.
`setFullscreen()` is `SDL_SetWindowFullscreen()` and `isFullscreen()` reads `SDL_WINDOW_FULLSCREEN` back off the window's flags, latched at `close()` like the size.

The one thing neither backend can correct for is a window system reporting a pointer in coordinates its window does not report a size in, which a high-DPI display can produce: SDL's mouse events are in window coordinates and `SDL_GetWindowSizeInPixels()` is in pixels, and the two coincide on every configuration this project builds for.
A backend where they did not would need a size of its own to map against, not a different rule above it.

`raylib` keeps its window flags in globals that outlive a window, so it sets *and clears* `FLAG_WINDOW_RESIZABLE` rather than only setting it — otherwise one resizable window would make every later one resizable.
Fullscreen is the same shape of problem once more: raylib offers `ToggleFullscreen()` rather than a setter, so `setFullscreen()` compares against `IsWindowFullscreen()` before it acts, and a caller asking twice for the state it already has gets it rather than the opposite of it.
It has no event queue, so `Resized` is synthesised by diffing the reported size against the last one reported, which is why the size itself is the latch rather than `IsWindowResized()`.
It still reports `maxWindows() == 1`.

## Why `configuredSize()` is the one member of `IWindow` that is not pure

Every other member is pure virtual, per the style guide.
`configuredSize()` has a default that answers with `size()`, because that answer is correct for anything with no window system behind it to disagree with — a test double, or a window that cannot be resized — and because making it pure would have forced a change on every existing implementation of the interface for no behavioural gain.
All three real backends override it.

## What the conformance suite does and does not check

`GfxBackendConformance` deliberately asserts nothing about the exact size a window *reports*, because a real window manager is free to resize a window as it appears, and requiring otherwise would force an honest backend to lie.
`configuredSize()` is the exception and the reason it is worth having: it is a number the caller chose, so it is the one window size every backend can be held to exactly, including per-window when the backend allows more than one.

Nor does the suite assert that a resizable window can actually be resized.
There is no display to drag an edge on, and `null` has no window system to honour the flag with at all.
What a backend is held to instead is that it accepts the request, reports the configured size exactly, reports a non-zero drawable size, and goes on answering both after `close()`.

Fullscreen is held to that much and no more, for the same reasons and one of its own: nothing asserts a window really covers a screen, since there is no screen, and nothing asserts what `isFullscreen()` reports either, because a window manager may refuse the request and an honest backend has to be free to say so.
What is asserted is that `setFullscreen()` is accepted in both directions and twice over, that it never moves `configuredSize()`, that the window is still open and still reporting a non-zero size afterwards, and that doing it to a closed window is harmless.
Backends that cannot do multiple windows skip the per-window test with `GTEST_SKIP()` rather than failing it, the same way the existing multi-window tests treat `maxWindows() == 1`.
