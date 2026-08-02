# antwika::font

`src/libs/font/` — glyph metrics and coverage masks, and nothing else.

## What it is for

`antwika::font` turns the bytes of a TrueType font into metrics and coverage masks.
It parses a font (`Font`, `TtfReader`), answers what the font says about a line and about one glyph (`FontMetrics`, `GlyphMetrics`), draws a glyph into an 8-bit mask (`Glyph`, `Coverage`), and packs a set of characters into one mask with the map from a character to the rectangle holding it (`GlyphAtlas`, `AtlasGlyph`, `makeGlyphAtlas`).
That is the whole of it.

It does **not** shape text, break lines, lay out paragraphs, cache anything, open a file, or talk to a renderer.
The four decisions behind that shape are each the kind that is expensive to reverse, so they are set out below.

## The library is named for the domain, the reader for the format

This was `antwika::ttf` first, and the name was a promise the library had no reason to make.
What it hands out is metrics and coverage masks, and neither of those is a TrueType concept: an OpenType font with CFF outlines answers exactly the same questions through a different outline path, so supporting one would be a second reader inside this library rather than a second library beside it.
A name that says `ttf` would have to be either broken or lived with at that point, and renaming a library is cheapest before anything links it.

So the library, the namespace and the CMake target are `font`, and `TtfReader` keeps its name, because it really does read a TrueType file specifically -- exactly as `gfx::PngReader` and `sound::WavReader` sit inside libraries named for what they are for rather than for one file format.
`FontError` follows the same reading in the other direction: it names the library's one failure category, as `gfx::GfxError` and `sound::SoundError` do, so it was `TtfError` for the wrong reason and is not now.
**None of this changed a refusal.**
A font collection and an OpenType font with CFF outlines are still turned away by name, with the same message and the same type; what stopped is the name claiming that is all this library could ever do.

## It depends on nothing, and the output type is why

The obvious thing to hand back from `rasterise()` is a `gfx::Bitmap`, since a texture is where a glyph is going.
`Coverage` is a value type of this library's own instead, and the argument has two halves.

**A glyph has one channel.**
A mask says how much ink landed on a pixel and says nothing about colour: what colour it comes out is the tint's answer at the moment it is drawn, which is the same call `apps/poker` already makes for its white rank and suit glyphs.
Storing four bytes where one carries the information would quadruple an atlas and would bake a decision into it that belongs at the draw call.

**And a dependency would be permanent.**
`antwika::gfx` pulls in GLM and `antwika::log`, so a `font` that names `gfx::Bitmap` for one struct of three fields has taken all of that on, and every future caller pays it.
As it stands the library depends on no other module of this project at all -- there is no `DEPENDS` line in its `CMakeLists.txt` -- which is `antwika::animation`'s and `antwika::wfc`'s shape, and it is what lets the whole of it be tested with no window, no renderer and no logger anywhere in sight.
`Rect` is unsigned and its own for the same reason, and because it addresses a mask rather than a window: there is nowhere off-screen for it to be.

The cost is real and it is four lines at the one seam that wants a texture:

```cpp
gfx::Bitmap bitmap{.size = {atlas.coverage.width, atlas.coverage.height}};
bitmap.pixels.reserve(atlas.coverage.samples.size() * gfx::kBytesPerPixel);

for (const std::uint8_t sample : atlas.coverage.samples)
{
    bitmap.pixels.insert(bitmap.pixels.end(), {255, 255, 255, sample});
}
```

White with the coverage in alpha, so `drawTexture`'s tint is what picks the colour -- one texture for every colour of text the application ever draws.
Four lines in the application that owns both libraries is a better place for that than a dependency in the library that owns neither.

## Bytes in, never a path

`TtfReader::read()` takes a `std::istream`, exactly as `gfx::PngReader::read()` and `sound::WavReader::read()` do, and `Font`'s constructor takes a `std::vector<std::uint8_t>`.
Nothing in the library opens a file; an application does, with `app::assetPath()`, as `apps/gfx_demo` does for its PNG.

The immediate reason is layering, and the reason that actually decides it is testing.
Every refusal this library can produce is reachable from bytes held in memory, so every one of them is provable without a fixture on disk -- which is what makes the project's 100% coverage gate satisfiable here rather than merely aspirational.
A file-taking entry point would put "the file could not be opened" in the same error type as "these bytes are not a font", and only one of those two is about fonts.

**Nothing is checked in to test it with.**
`src/libs/font/tests/SyntheticFont.cpp` builds a whole, valid, four-glyph TrueType font in memory: an offset table, `cmap`, `glyf`, `head`, `hhea`, `hmtx`, `kern`, `loca` and `maxp`, written byte by byte.
A real font would be a licence to track, a megabyte in every clone and a fixture nobody could read a diff of -- and, decisively, a real font is by definition not malformed in any of the ways the refusals catch, so most of them would have stayed untested.
Every measurement in the tests is an exact pixel rather than a tolerance, because the synthetic font is 1000 units per em with an ascender of 800 and a descender of -200, so a change in how this library rounds is visible instead of absorbed.

## The rasteriser is behind a wall, and the wall is one file

`stb_truetype.h` arrives through the `stb` package this project already depends on for `stb_image`, so no new dependency and no lockfile change was involved.
`src/StbTrueType.cpp` is the one translation unit that compiles it, and `detail::Rasteriser` in `src/StbTrueType.hpp` is the only thing in the library that calls it.
`Font` holds a `std::unique_ptr<detail::Rasteriser>` to an incomplete type and declares its own destructor, which is `ecs::World`'s arrangement with `detail::EntityManager` -- so no public header of this library names a rasteriser, and neither can anything that includes one.

`STBTT_STATIC` is what makes that wall necessary rather than tidy.
raylib links its own copy of `stb_truetype`, and two sets of those symbols in one program do not link, which is the lesson `STB_IMAGE_STATIC` already taught in `antwika::gfx`.
Making them static also makes every entry point this library never calls an unused function, which `-Wunused-function` reports and `-Werror` promotes -- so that one file is compiled with `-w`, and that is exactly why it holds nothing but one-call wrappers.
Rounding, refusals and packing all live in `Font.cpp`, `FontDirectory.cpp` and `GlyphAtlas.cpp`, which the compiler is still allowed to complain about.

`FontDirectory.cpp` is the other half of the wall.
stb trusts the offsets it is handed, which is the usual bargain with a single-header decoder.
So the offset table and every table record are checked to lie inside the blob before the rasteriser sees any of it, and a record claiming a table that runs past the end is a `FontError` before anything reads a byte of it.
What it deliberately does not do is validate a table's *contents*, and the next section is what that costs.
A font collection and an OpenType font with CFF outlines are turned away by name, since those are the two wrong files somebody is most likely to be holding.

## Bundled fonts only, and this is a boundary rather than a check

**`Font` must only ever be fed a font the application itself ships.**
One bundled beside the executable with `antwika_bundle_app()`, or compiled in with `antwika_embed_binary()` as `assets/fonts/RobotoMono-Regular.ttf` is, is what this library is for.
A font a user picked, uploaded or downloaded is not, and no refusal this library makes changes that.

The reason is *where* a font is parsed, and the directory check above is only the first inch of it.
stb_truetype's own documentation states plainly that it does no range checking of the data it reads, and the reads that matter happen long after `TtfReader::read()` has returned successfully:

- a cmap subtable offset is followed when `Font::has()` or a glyph lookup asks about a codepoint,
- and `loca`/`glyf` extents are followed when a glyph is rasterised, one per character in a `makeGlyphAtlas()` call.

So a font crafted to pass the directory check reads out of bounds *mid-use* -- inside a frame being drawn, in a call whose declared failure mode is a `.notdef` box -- rather than refusing to open.
There is no `FontError` for it and there is not going to be one.

Validating those offsets here was considered and refused, on the grounds the rest of the wall is built on: it would be a second, weaker parser of the same tables kept in step with stb's by hand, and every gap between the two would read as safety while being none.
The honest boundary is this sentence instead.
In-tree exposure is one font, checked in, compiled into `antwika::gfx`; an application that bundles another takes that decision in the open, and it is a decision about provenance rather than about parsing.

## Integers out, and lookup that answers

Every number this library hands out is a whole number of pixels at a requested pixel height, and no floating-point value crosses the public interface.
Scaling design units by a `float` happens once, inside `Font.cpp`'s `toPixels()`, because rounding twice is how a baseline ends up a row away from the glyphs that were rasterised against it.

That is a weaker guarantee than `antwika::animation`'s exact rational `Progress`, and deliberately so: an outline is floating point in the file, so exactness was never on the table here.
What matters is the rule it is serving, which is the one `gfx::Transform` and `gfx::Camera3D` follow -- **nothing this library returns may enter simulation state.**
Text is drawn, and drawing is a write-only projection, so a glyph box that differs in its last bit between two toolchains costs a pixel and never a divergent replay.
A layout that a click has to resolve against is `apps/game`'s camera problem, not this one, and it is solved there by holding whole tile sizes rather than a scale factor.

Lookup is total.
A codepoint the font has no glyph for rasterises as glyph zero -- the `.notdef` box the font itself chose -- rather than throwing, following `antwika::i18n`'s rule that anything running while a frame is being drawn answers rather than fails.
`Font::has()` is how a caller asks in advance.
A blank glyph is an ordinary answer too: a space is an empty `Coverage` with a non-zero advance, and in an atlas it gets no rectangle and keeps its metrics, so laying a string out never has to ask which kind it got.

`FontError` is the one thing that is thrown, per the project's one-exception-type-per-failure-category rule, and it covers exactly two categories of caller mistake: bytes that are not a font this library reads, and an argument that has no answer -- a pixel height of zero, a sample outside a mask, an atlas of no characters, a glyph too wide for the atlas it was asked to fit.

## Packing is a shelf, on purpose

`makeGlyphAtlas()` sorts the characters, drops duplicates, and lays the glyphs out left to right, starting a new row when one will not fit.
That is not the tightest packing there is.
It is the one whose result depends on nothing but the arguments: the same request builds the same atlas byte for byte on every machine, and two atlases over the same characters compare equal whatever order they were asked for in.
Entries stay sorted by codepoint, which is also what makes `find()` a binary search rather than a second index to keep in step.

## What this is wired into

**Every line of text any application draws comes through here.**
`IRenderer::drawText()` is painted from `gfx::forEachGlyphPixel()`, which takes its ink from a `gfx::GlyphCells` — a `makeGlyphAtlas()` over printable ASCII, rasterised at the cell height `gfx::textSize()` measures and baked onto that cell grid.
The font is `assets/fonts/RobotoMono-Regular.ttf`, compiled into `antwika::gfx` by `antwika_embed_binary()` rather than opened, since this library takes bytes and never a path and `antwika::gfx` has no application to ask where a file went.

**What did not happen is the half worth reading.**
`gfx::textSize()` did not move, and neither did one `antwika::ui` layout, one hit test or one recorded click: a character still occupies exactly `kGlyphAdvance` by `kGlyphLineHeight` times its scale, and nothing this library returns is allowed anywhere near that arithmetic.
This is the rule stated above — *nothing this library returns may enter simulation state* — being kept at the one seam where it would have been most convenient to break, and [`wiki/libraries/gfx.md`](gfx.md) sets out the reasoning from the other side.
An application that wants a real font's *metrics*, rather than a real font's *look*, still asks this library directly, bundles its own `.ttf` with `antwika_bundle_app()`, and takes on the decision in the open -- a font it ships, never one it was handed, for the reason set out above.

The other route in is unchanged and is what an application *would* use to draw text at a size the fixed cell has no answer for: `makeGlyphAtlas()` hands back one mask and one rectangle per character, `gfx::glyphAtlasBitmap()` expands the mask to straight RGBA, `IRenderer::createTexture()` uploads it once and `gfx::AtlasText.hpp` measures and blits from it.

**Nothing in this tree takes that route, and nothing calls `Font::kerning()` either.**
Both are speculative surface — tested to the same 100% as everything else here, and adopted by nobody — which is worth stating outright rather than leaving a reader to infer use from a page that describes a mechanism, following the precedent [`tween`](tween.md) sets by listing the applications that do not use it and why.
`kerning()` in particular is waiting for a caller that lays text out proportionally: the fixed cell `gfx::textSize()` measures advances every character by the same amount, so there is nowhere in the one live text path for a pair adjustment to go.
Neither is deleted, because both are answers this library owes anything asking it for real metrics rather than for the built-in font's look, and a `kern` table this cannot read is a font this cannot fully describe.
