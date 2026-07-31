# apps/gfx3d_demo

`src/apps/gfx3d_demo/` — the 3D half of the graphics showcase.

## What it demonstrates

[`gfx`](../libraries/gfx.md)'s `IRenderer3D`: a cube uploaded as a mesh, turned once per frame, with a caption drawn over it through the ordinary 2D calls.
One frame, both halves.

## Running it

```sh
build/bin/antwika_gfx3d_demo
```

It draws a fixed number of frames and stops, rather than running until the window closes.
That is deliberate: the default `null` backend reports no close and is the build every CI leg produces, so an uncapped run there would never finish.

`sdl3` inherits the null 3D default and reports no 3D renderer, so this app raises a `GfxError` under it.
`raylib` implements the seam.

## Libraries it composes

[`app`](../libraries/app.md), [`gfx`](../libraries/gfx.md), [`log`](../libraries/log.md), plus the selected graphics backend.

## How it is put together

`CubeMesh` supplies the geometry as a `gfx::MeshData`, `SpinScene` turns a tick number into a model matrix and a camera, and `SpinLoop` opens the window, uploads the mesh once, and draws.

## Non-obvious decisions

**The turn is a function of the frame count and never of a clock.**
The same frame is the same picture on every machine, which is the same rule [`animation`](../libraries/animation.md) exists to enforce.
A cube spun by elapsed wall-clock time would look identical and be untestable.

**The tick count lives in the loop rather than in the scene.**
That is what lets the scene stay a pure function of what it is handed, so a frame can be asserted against a mock renderer with no window at all.

**A backend with no 3D path says so rather than dropping the draw.**
`IRenderer::renderer3d()` is non-pure and returns null by default, so every existing implementer kept compiling when 3D arrived — and this app gets an error it can report rather than an empty window it cannot explain.

**The maths types here are floating point, and that is safe.**
`gfx::Transform` and `gfx::Camera3D` are render-side only.
Floating point may never appear in anything a replay reproduces, and rendering is already a write-only projection — which is exactly why [game](game.md)'s camera is *not* one of these types.

## See also

- [`blog/012-a-window-that-cant-talk-back.md`](../../blog/012-a-window-that-cant-talk-back.md).
