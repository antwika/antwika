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
| `IRenderer.hpp` | `IRenderer` | Clear, fill rectangles, draw one-pixel lines, draw text, `createTexture()`, `drawTexture()`, present, `detach()`. |
| `ITexture.hpp` | `ITexture` | An opaque uploaded image. |
| `Bitmap.hpp`, `PngReader.hpp`, `Blit.hpp` | `Bitmap`, `PngReader` | Decoding a byte stream to straight RGBA, once, in the library rather than per backend. |
| `Glyphs.hpp`, `TextLayout.hpp` | `textSize()` | The one built-in fixed-cell bitmap font and its metrics. |
| `Point.hpp`, `Size.hpp`, `Rect.hpp`, `Color.hpp` | — | Geometry and colour. |
| `NullBackend.hpp` | `NullBackend` | The headless backend: opens windows that draw nothing, needs no display. |
| `SelectedBackend.hpp` | — | Resolves the backend chosen at configure time. |
| `GfxError.hpp` | `GfxError` | One error type, raised identically by every backend. |

A conformance suite lives under `tests/conformance/` (`GfxBackendConformance.hpp`), and every backend runs it.
`MockGfxBackend`, `MockRenderer`, `MockWindow` and `MockTexture` live under `tests/mocks/`.

## Depends on

[`log`](log.md) only.
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

See [`blog/012-a-window-that-cant-talk-back.md`](../../blog/012-a-window-that-cant-talk-back.md).
