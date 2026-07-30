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

## Per backend

`null` accepts `resizable` and honours it by having nothing act on it: with no window system present, `size()` and `configuredSize()` are equal forever.
That is the deliberate headless behaviour, not an omission — a headless run must be the one where the reported size cannot surprise anybody.

`sdl3` passes `SDL_WINDOW_RESIZABLE` when asked, reads the live drawable size through `SDL_GetWindowSizeInPixels()` on every `size()` call, and emits `gfx::Resized` from `SDL_EVENT_WINDOW_RESIZED`.
It latches the last size it saw at `close()`, so a closed window still answers.

`raylib` keeps its window flags in globals that outlive a window, so it sets *and clears* `FLAG_WINDOW_RESIZABLE` rather than only setting it — otherwise one resizable window would make every later one resizable.
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
Backends that cannot do multiple windows skip the per-window test with `GTEST_SKIP()` rather than failing it, the same way the existing multi-window tests treat `maxWindows() == 1`.
