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
| `IRenderer.hpp` | `IRenderer` | Clear, fill rectangles, draw one-pixel lines, draw text, `createTexture()`, `drawTexture(texture, source, destination, tint)`, present, `detach()`, `renderer3d()`. |
| `ITexture.hpp` | `ITexture` | An opaque uploaded image. |
| `IRenderer3D.hpp` | `IRenderer3D` | `createMesh()` and `drawMesh(mesh, model, camera, tint)`. |
| `IMesh.hpp`, `MeshData.hpp` | `IMesh`, `MeshData` | An opaque uploaded mesh, and the vertices and indices it was built from. |
| `Math3D.hpp` | `Vec3`, `Mat4` | GLM, aliased rather than wrapped. |
| `Transform.hpp`, `Camera3D.hpp` | `Transform`, `Camera3D` | A model transform, and a perspective or orthographic camera. |
| `Bitmap.hpp`, `PngReader.hpp`, `Blit.hpp` | `Bitmap`, `PngReader` | Decoding a byte stream to straight RGBA, once, in the library rather than per backend. |
| `PngWriter.hpp` | `PngWriter` | `write(bitmap, out)` — the way back out, to a stream rather than a path. |
| `Glyphs.hpp`, `TextLayout.hpp` | `kGlyphAdvance`, `kGlyphLineHeight`, `textSize()` | The one built-in font's fixed cell, and the arithmetic over it. |
| `GlyphCells.hpp`, `TextRaster.hpp` | `GlyphCells`, `glyphCells()`, `forEachGlyphPixel()` | That font rasterised onto those cells, and the one walk over the result every backend paints. |
| `Point.hpp`, `Size.hpp`, `Rect.hpp`, `Color.hpp` | — | Geometry and colour. |
| `NullBackend.hpp` | `NullBackend` | The headless backend: opens windows that draw nothing, needs no display. |
| `SelectedBackend.hpp` | — | Resolves the backend chosen at configure time. |
| `GfxError.hpp` | `GfxError` | One error type, raised identically by every backend. |

A conformance suite lives under `tests/conformance/` (`GfxBackendConformance.hpp`), and every backend runs it.
`MockGfxBackend`, `MockRenderer`, `MockWindow` and `MockTexture` live under `tests/mocks/`.

## Depends on

[`font`](font.md) and [`log`](log.md), plus `glm` (PUBLIC, for the 3D maths types) and `stb` (PRIVATE, for PNG decoding).

`font` is PUBLIC because `AtlasText.hpp` and `GlyphAtlasBitmap.hpp` name `font::GlyphAtlas` in their signatures.
The direction is the only one available: `antwika::font` names no module of this project at all and has no `DEPENDS` line to name one with, so `gfx -> font` is acyclic by construction rather than by agreement.

It deliberately does **not** depend on [`input`](input.md), and `input` does not depend on it: reading input must not require opening a window.

## Non-obvious decisions

**Drawing is write-only, by omission.**
There is no pixel read-back, no render target and no screenshot anywhere in the interface, because read-back is the one thing that would let a picture feed a simulation and break replay.
`ITexture` is opaque for the same reason.
Rendering is therefore a projection of state that never feeds back into the tick loop, which is what keeps a replay reproducible under the headless `NullBackend`.

**A `Color` is straight alpha, and every drawing call blends with it.**
`clear()` alone is excepted, since that one replaces the drawable area rather than drawing over it.
What a framework does by default is not the answer: SDL's renderer starts at `SDL_BLENDMODE_NONE` and has to be asked, where raylib blends already — which is exactly how one backend came to paint a tower's translucent range as a flat opaque blue.
That is the whole of what the shared conformance suite can say about it, since `IRenderer` reports no pixel back, so `backends/sdl3/tests/Sdl3RendererTest.cpp` is where the result itself is asserted — through SDL's own read-back, at the backend's own seam, rather than by inventing one above it.

**Text is arithmetic, not measurement.**
One built-in fixed-cell font is defined by this library and drawn identically by every backend, so a caller (notably [`ui`](ui.md)) lays text out from `textSize()` alone and never asks a backend how wide something is.
A second font would imply per-backend glyph metrics, which is exactly why loading fonts is out of scope while loading textures is not — a decoded bitmap has no metrics for a backend to disagree about.

**The glyphs are Roboto Mono; the cells they are drawn into are not.**
The ink inside a cell used to be a 5x7 table of bits written out in `Glyphs.cpp`, and is now a real typeface rasterised through [`font`](font.md): `GlyphCells` builds a `font::GlyphAtlas` at `kGlyphLineHeight * scale` pixels and bakes each character onto its own cell, and `forEachGlyphPixel()` walks the result.
**Not one number `textSize()` reports moved, and that is the whole shape of the change.**
The pen still steps `kGlyphAdvance * scale` per character and a line is still `kGlyphLineHeight * scale` tall, so every layout in the tree — and every click an application resolves against one — is byte for byte what it was.

That is not a coincidence to be grateful for; it is the constraint the design is built around, and [`AtlasText.hpp`](../../src/libs/gfx/include/antwika/gfx/AtlasText.hpp) writes the argument out in full.
A real font's metrics are whole pixels scaled from an outline by a `float`, and a value differing in its last bit between two toolchains costs a pixel where it is drawn and costs a divergent session where a click is resolved against it.
So no metric of the embedded font reaches a layout: the cell is `6` by `8` times a whole scale, both frozen in `Glyphs.hpp`, and the font is only ever asked what a cell should look like.
Ink is clipped to its cell rather than trusted to fit it, so no character can light a pixel outside the box `textSize()` reports whatever the font says about a glyph — the one property a `ui` widget's neighbour depends on.

Rasterising at `kGlyphLineHeight * scale` is the font's own answer rather than a fitting rule invented here.
At that height Roboto Mono's ascender-to-descender *is* the cell, and the printable ASCII range's ink measures exactly the 8 rows of a scale-1 cell and stays inside every larger one.
A rule that grew the glyphs to fill the cell better was measured and rejected: it buys 16-22% at scales 2 to 4, nothing at scale 1, and costs a search whose result is one more number that could disagree with `textSize()`.

**The coverage is 8 bits, and the colour a backend fills with is worked out here.**
`forEachGlyphPixel()` takes the text colour and hands the visitor a `Rect` and the colour to fill it with, coverage already folded into the alpha by `glyphPixelColor()`.
Thresholding to one bit was the alternative, and it keeps SDL's one batched `SDL_RenderFillRects` per line rather than one per colour — but at scale 1 an 8-pixel-tall Roboto Mono thresholded at 128 is genuinely unreadable, which would have made "text now uses a real font" a regression for every application at small canvas sizes.
Handing the colour over rather than the coverage is deliberate: a backend that had to fold coverage into alpha itself is a backend that could fold it in differently, and this file exists so that no backend gets that latitude.

**The font is compiled in, and there is nothing to find at run time.**
`antwika::gfx` cannot ask `antwika::app` where an asset lives — `app` depends on `gfx` — and it opens no files at all, for the reason `PngReader` takes a stream.
So `antwika_embed_binary()` in [`cmake/AntwikaEmbedBinary.cmake`](../../cmake/AntwikaEmbedBinary.cmake) turns `assets/fonts/RobotoMono-Regular.ttf` into a C++ source of bytes at configure time, and `BuiltInFont.cpp` parses it once.
The generated source is data and nothing else — no function, no initialiser that runs — so it adds nothing for the coverage gate to measure, and it is written by CMake itself rather than by a tool the build has to run, which is what keeps a cross build to MinGW from needing a host-built generator.
An application that wants a real font *of its own* still does what [`font`](font.md) says: bundles it with `antwika_bundle_app()`, opens it with `app::assetPath()` and draws it through `AtlasText.hpp`.

**The cells are cached per scale, in a static, on purpose.**
Text is drawn every frame and rasterising 95 glyphs is not frame work, so `glyphCells(scale)` builds one set of cells the first time a scale is drawn at and keeps it in a function-local `std::map` — a `map` rather than a `vector` because a reference handed out has to survive the arrival of every later scale.
It is mutable global state and it is worth being exact about why that is acceptable here rather than pretending otherwise: what is kept is a memo of a pure function of the scale and of bytes the build compiled in, nothing can reach it, empty it or replace what is in it, and it can therefore change how long a call takes and nothing else.
Drawing is a write-only projection, so nothing it hands back is ever read back into a tick.
The numbers are why it is not an argument: building one scale's cells costs 0.23 to 0.44 ms and reaching cached ones costs about 50 ns, so an atlas per `drawText()` would be a quarter of a millisecond per line of text on a 16 ms frame.
The other alternative — a cache each backend owns — would put the one thing every backend has to agree about in the one place they are allowed to differ.
Walking one 57-character line costs 5 µs at scale 1, 18 µs at scale 2 and 42 µs at scale 3, which is the cost of about 2.7 times as many rectangles as the bitmap font emitted; a backend's own fill calls are on top of that and are what the batching in `Sdl3Renderer::drawText()` is for.

**Images are decoded once, by the library.**
`PngReader::read()` takes a byte stream, not a path, so `gfx` opens no files and every decoder failure is provable headlessly; the app supplies the bytes.
An app is what opens the file, as [`gfx_demo`](../apps/gfx_demo.md) does with `app::assetPath()`, which finds the PNG shipped in the application's own directory under `bin/`, and as [`atlas_editor`](../apps/atlas_editor.md) does with its `PngAtlasStore`.
stb_image is compiled `STB_IMAGE_STATIC` in one translation unit, because raylib links its own copy.
`IRenderer::createTexture()` is what uploads the decoded bitmap, and `drawTexture(texture, source, destination, tint)` blits part of it with a colour and alpha modulation.

**Writing a PNG is on exactly the same terms.**
`PngWriter::write(bitmap, out)` takes a stream rather than a path, so every refusal it can produce is reachable from an in-memory stream and provable with no fixture on disk, and its argument order is `ReplayWriter`'s.
It flushes before it checks the stream, because a file small enough to sit in one buffer would otherwise be refused by the filesystem and thrown away in silence — a save that loses a sheet quietly is the one failure an editor cannot recover from.
stb_image_write is compiled in one translation unit of its own (`STB_IMAGE_WRITE_STATIC`, `STBI_WRITE_NO_STDIO`, warnings off) for the reasons `StbImage.cpp` gives.

**A texture belongs to the renderer that made it.**
Drawing it through another draws nothing, and it may safely outlive its window, because each renderer's `detach()` frees its live textures before the framework tears the device down.

**A backend declares its window limit rather than being required to have none.**
raylib reports `maxWindows() == 1`, since it keeps its one window in global state, and the conformance suite skips its multi-window tests for such a backend instead of failing them.

**3D is a sibling interface, not more methods on `IRenderer`.**
`IRenderer::renderer3d()` is non-pure and returns null by default, so a backend with no 3D path says so rather than accepting a draw and dropping it — and every existing implementer, backends and test doubles alike, kept compiling unchanged when it arrived.
`clear()` and `present()` stay on `IRenderer` because there is one frame that both halves draw into.
`IMesh` mirrors `ITexture` exactly: opaque, owned by the renderer that made it, and with no read-back of any kind.

`null` and `raylib` implement `IRenderer3D`; `sdl3` inherits the null default and reports no 3D renderer, which is a conforming answer rather than a gap.
The conformance suite covers the 3D calls and skips every one of them for a backend that offers none.

**The raylib 3D backend sets the matrices itself rather than handing raylib a camera.**
It goes through `rlgl` (`rlSetMatrixProjection`/`rlSetMatrixModelview`) instead of passing a `::Camera3D` to `BeginMode3D()`, because that struct describes a projection by a field of view and picks its own clip planes: a `gfx::Camera3D`'s near and far planes — and an orthographic one's extents — would be discarded and quietly replaced, nothing would fail, and the scene would simply be wrong.
`RaylibMesh` copies `RaylibTexture`'s ownership rules exactly, and `RaylibMaterial` wraps raylib's default material, which `DrawMesh` insists on being handed and which the tint is set on.
raylib indexes a mesh with 16-bit indices where `MeshData` says 32, so a mesh with more vertices than one of those can address is refused with a `GfxError` rather than silently wrapped around.

**The 3D maths types are render-side only.**
They are floating point, and floating point may never appear in anything a replay reproduces.
That costs nothing, because rendering is already a write-only projection — and it is exactly why [game](../apps/game.md)'s camera is *not* one of these types but integer simulation state.

**A resizable window has two sizes, and they are named apart.**
`IWindow::configuredSize()` is the size the app asked for and is the same number on the recording machine and the replaying one; `IWindow::size()` is what the window currently reports.
**Nothing in a simulation may be driven from the reported size** — laying out or hit-testing against it would make a window resize change what a recorded click means — so it is only ever used to place what is drawn inside the drawable area.

See [`blog/012-a-window-that-cant-talk-back.md`](../../blog/012-a-window-that-cant-talk-back.md) and [`docs/resizable-windows.md`](../../docs/resizable-windows.md).
[`apps/gfx_demo`](../apps/gfx_demo.md) is the 2D showcase and [`apps/gfx3d_demo`](../apps/gfx3d_demo.md) the 3D one.
