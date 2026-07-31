# antwika::gfx

`src/libs/gfx/` — windows and drawing, with no framework named.

## What it is for

Opening windows and drawing into them through an abstraction, so no file under `src/` mentions SDL or raylib.
The concrete frameworks live under `backends/`, and exactly one is compiled into a build.

## Key types

| Header | Type | Role |
| --- | --- | --- |
| `IGfxBackend.hpp` | `IGfxBackend` | Creates windows, polls events, reports `maxWindows()`. |
| `IWindow.hpp`, `WindowDesc.hpp`, `WindowId.hpp` | `IWindow`, `WindowDesc`, `WindowId` | A window, how it was asked for, and its identity. |
| `WindowEvent.hpp` | `WindowEvent`, `CloseRequested`, `Resized` | A `std::variant` payload plus the `WindowId` it happened to. |
| `IRenderer.hpp` | `IRenderer` | Clear, fill rectangles, draw one-pixel lines, draw text, `createTexture()`, `drawTexture()`, present, `detach()`, `renderer3d()`. |
| `ITexture.hpp` | `ITexture` | An opaque uploaded image. |
| `IRenderer3D.hpp` | `IRenderer3D` | `createMesh()` and `drawMesh(mesh, model, camera, tint)`. |
| `IMesh.hpp`, `MeshData.hpp` | `IMesh`, `MeshData` | An opaque uploaded mesh, and the vertices and indices it was built from. |
| `Math3D.hpp` | `Vec3`, `Mat4` | GLM, aliased rather than wrapped. |
| `Transform.hpp`, `Camera3D.hpp` | `Transform`, `Camera3D` | A model transform, and a perspective or orthographic camera. |
| `Bitmap.hpp`, `PngReader.hpp`, `Blit.hpp` | `Bitmap`, `PngReader` | Decoding a byte stream to straight RGBA, once, in the library rather than per backend. |
| `Glyphs.hpp`, `TextLayout.hpp` | `textSize()` | The one built-in fixed-cell bitmap font and its metrics. |
| `Point.hpp`, `Size.hpp`, `Rect.hpp`, `Color.hpp` | — | Geometry and colour. |
| `NullBackend.hpp` | `NullBackend` | The headless backend: opens windows that draw nothing, needs no display. |
| `SelectedBackend.hpp` | — | Resolves the backend chosen at configure time. |
| `GfxError.hpp` | `GfxError` | One error type, raised identically by every backend. |

A conformance suite lives under `tests/conformance/` (`GfxBackendConformance.hpp`), and every backend runs it.
`MockGfxBackend`, `MockRenderer`, `MockWindow` and `MockTexture` live under `tests/mocks/`.

## Depends on

[`log`](log.md), plus `glm` (PUBLIC, for the 3D maths types) and `stb` (PRIVATE, for PNG decoding).

It deliberately does **not** depend on [`input`](input.md), and `input` does not depend on it: reading input must not require opening a window.

## Non-obvious decisions

**Drawing is write-only, by omission.**
There is no pixel read-back, no render target and no screenshot anywhere in the interface, because read-back is the one thing that would let a picture feed a simulation and break replay.
`ITexture` is opaque for the same reason.

**Text is arithmetic, not measurement.**
One built-in fixed-cell bitmap font is defined by this library and drawn identically by every backend, so a caller (notably [`ui`](ui.md)) lays text out from `textSize()` alone and never asks a backend how wide something is.
A second font would imply per-backend glyph metrics, which is exactly why loading fonts is out of scope while loading textures is not — a decoded bitmap has no metrics for a backend to disagree about.

**Images are decoded once, by the library.**
`PngReader::read()` takes a byte stream, not a path, so `gfx` opens no files and every decoder failure is provable headlessly; the app supplies the bytes.
stb_image is compiled `STB_IMAGE_STATIC` in one translation unit, because raylib links its own copy.

**A texture belongs to the renderer that made it.**
Drawing it through another draws nothing, and it may safely outlive its window, because each renderer's `detach()` frees its live textures before the framework tears the device down.

**A backend declares its window limit rather than being required to have none.**
raylib reports `maxWindows() == 1`, since it keeps its one window in global state, and the conformance suite skips its multi-window tests for such a backend instead of failing them.

**3D is a sibling interface, not more methods on `IRenderer`.**
`IRenderer::renderer3d()` is non-pure and returns null by default, so a backend with no 3D path says so rather than accepting a draw and dropping it — and every existing implementer, backends and test doubles alike, kept compiling unchanged when it arrived.
`clear()` and `present()` stay on `IRenderer` because there is one frame that both halves draw into.

`null` and `raylib` implement `IRenderer3D`; `sdl3` inherits the null default and reports no 3D renderer, which is a conforming answer rather than a gap.
The conformance suite covers the 3D calls and skips every one of them for a backend that offers none.

**The 3D maths types are render-side only.**
They are floating point, and floating point may never appear in anything a replay reproduces.
That costs nothing, because rendering is already a write-only projection — and it is exactly why [game](../apps/game.md)'s camera is *not* one of these types but integer simulation state.

**A resizable window has two sizes, and they are named apart.**
`IWindow::configuredSize()` is the size the app asked for and is the same number on the recording machine and the replaying one; `IWindow::size()` is what the window currently reports.
**Nothing in a simulation may be driven from the reported size** — laying out or hit-testing against it would make a window resize change what a recorded click means — so it is only ever used to place what is drawn inside the drawable area.

See [`blog/012-a-window-that-cant-talk-back.md`](../../blog/012-a-window-that-cant-talk-back.md) and [`docs/resizable-windows.md`](../../docs/resizable-windows.md).
