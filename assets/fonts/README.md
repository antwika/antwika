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
[`src/apps/game/assets/atlas.png`](../../src/apps/game/assets/atlas.png) is.
Production code loads it; no test does.
A new test that wants a font asks `SyntheticFont` for one.

## How an application picks it up

The same way it picks up an atlas, which is the only way this project ships a file beside a binary:

```cmake
antwika_bundle_app(TARGET antwika_my_app
    ASSETS ${CMAKE_SOURCE_DIR}/assets/fonts/RobotoMono-Regular.ttf
)
```

That copies the file into the application's own directory under `bin/`, and
`antwika::app::assetPath("RobotoMono-Regular.ttf")` is what finds it at run time -- never a path baked in at configure time, which would be the building machine's path rather than the running machine's.
The application then opens it, hands the bytes to `font::TtfReader`, builds a `font::GlyphAtlas`, expands that into a `gfx::Bitmap` with `gfx::glyphAtlasBitmap()` and uploads it once with `IRenderer::createTexture()`.
`gfx::AtlasText.hpp` is what draws and measures from the result.
