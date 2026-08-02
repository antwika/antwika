# Bundled fonts

`RobotoMono-Regular.ttf` is Roboto Mono 3.001, taken verbatim from
[`googlefonts/RobotoMono`](https://github.com/googlefonts/RobotoMono)
(`fonts/ttf/RobotoMono-Regular.ttf`).
`LICENSE.txt` is that repository's `OFL.txt`, copied byte for byte: the family is licensed under the SIL Open Font License 1.1, which is what the font's own name table says too, so nothing here is Apache-2.0 despite older write-ups of the project saying so.
Copying a licence under a different file name is allowed; editing one is not, and this one is not edited.

## This is a shipped asset, not a test fixture

**`antwika::font` still checks in no font binary, and this file does not change that.**
`src/libs/font/tests/SyntheticFont.cpp` builds a whole valid four-glyph TrueType font in memory, byte by byte, and it stays the only thing the library's tests read.
That is deliberate and load-bearing rather than an accident somebody has now tidied up: every refusal `Font`, `TtfReader` and `FontDirectory` can produce is reachable from bytes held in memory, which is what makes the project's 100% coverage gate satisfiable there, and a real font is by definition not malformed in any of the ways those refusals catch.
See [`wiki/libraries/font.md`](../../wiki/libraries/font.md) for the argument in full.

What this directory holds is the other thing: the art an application draws with, in exactly the sense
[`src/apps/game/assets/atlas_1x1.png`](../../src/apps/game/assets/atlas_1x1.png) is.
Production code loads it; no test does.
A new test that wants a font asks `SyntheticFont` for one.

## `antwika::gfx` compiles it in, and that is where the text you see comes from

This is the font `IRenderer::drawText()` draws, which makes it the font of every application in the tree.
It gets there without being shipped anywhere: `antwika_embed_binary()` in
[`cmake/AntwikaEmbedBinary.cmake`](../../cmake/AntwikaEmbedBinary.cmake)
turns this file into a C++ source of bytes at configure time, and `antwika::gfx` parses that once and rasterises it onto the fixed cells `gfx::textSize()` measures.

The reason it is embedded rather than bundled is layering: `antwika::gfx` cannot call `antwika::app::assetPath()`, since `app` depends on `gfx` and not the other way round, and it opens no files at all — the rule `PngReader` and `TtfReader` already follow.
A library with no application to ask has nowhere to look, so the bytes have to arrive with the binary.
See [`wiki/libraries/gfx.md`](../../wiki/libraries/gfx.md) for what that costs and what it buys.

## How an application picks it up for itself

An application wanting this font at a size the fixed cell has no answer for — or wanting a font of its own — bundles it the same way it picks up an atlas, which is the only way this project ships a file beside a binary:

```cmake
antwika_bundle_app(TARGET antwika_my_app
    ASSETS ${CMAKE_SOURCE_DIR}/assets/fonts/RobotoMono-Regular.ttf
)
```

That copies the file into the application's own directory under `bin/`, and
`antwika::app::assetPath("RobotoMono-Regular.ttf")` is what finds it at run time -- never a path baked in at configure time, which would be the building machine's path rather than the running machine's.
The application then opens it, hands the bytes to `font::TtfReader`, builds a `font::GlyphAtlas`, expands that into a `gfx::Bitmap` with `gfx::glyphAtlasBitmap()` and uploads it once with `IRenderer::createTexture()`.
`gfx::AtlasText.hpp` is what draws and measures from the result.

No application in this tree does any of that yet: every one of them lays text out on the fixed cell `gfx::textSize()` measures, so this section is a route that exists rather than one anything walks — see [`wiki/libraries/gfx.md`](../../wiki/libraries/gfx.md).
