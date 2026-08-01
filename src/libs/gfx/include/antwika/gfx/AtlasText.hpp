#pragma once

#include <string_view>
#include <vector>

#include <antwika/font/GlyphAtlas.hpp>

#include "antwika/gfx/Color.hpp"
#include "antwika/gfx/GlyphBlit.hpp"
#include "antwika/gfx/IRenderer.hpp"
#include "antwika/gfx/ITexture.hpp"
#include "antwika/gfx/Point.hpp"
#include "antwika/gfx/Size.hpp"

namespace antwika::gfx
{

    /**
     * @brief Measure the area a line of text drawn from a real font's
     * atlas will occupy.
     *
     * Arithmetic over what the atlas already holds rather than a
     * question for a backend, exactly as textSize() is -- and this is
     * the header where the rule the whole path is held to is written
     * down, since measuring is the half of it that could do damage.
     *
     * **textSize() and IRenderer::drawText() are untouched by any of
     * this, and that is the shape of the change.** The built-in
     * fixed-cell font is still what antwika::ui lays out from and still
     * what every backend draws, so every layout in the tree -- and
     * every click an application resolves against one -- is byte for
     * byte what it was.  A caller opts in to a real font by calling one
     * of these three functions, and nothing opts in on its behalf.
     *
     * **What is measured here may decide what is drawn and nothing
     * else**, which is input::PointerHintChannel's rule arrived at from
     * the other side, and which wiki/libraries/font.md already states
     * for antwika::font: nothing that library returns may enter
     * simulation state.
     *
     * Being exact about why is worth the lines, because everything here
     * is integer arithmetic and so looks safe enough to lay a
     * hit-tested UI out with.  A font::GlyphAtlas is a pure function of
     * the font bytes, the pixel height and the packing options, and
     * antwika::font rounds a scaled design unit to a whole pixel
     * exactly once -- so two machines running one build over one
     * shipped .ttf do reach the same numbers in practice.  In practice
     * is not the guarantee a replay needs.  The scale factor behind
     * those whole pixels is a float, taken from an outline the file
     * holds in floating point, and a value differing in its last bit
     * between two toolchains costs a pixel where it is drawn and costs
     * a divergent session where a click is resolved against it.
     *
     * So a hit-testable layout carries on being laid out from
     * textSize(), whose metrics are whole numbers times whole numbers.
     * Wanting a real font's metrics in a layout a click resolves
     * against is a decision to take in the open -- pinning the font
     * bytes in the build and carrying the atlas's integer metrics into
     * simulation state deliberately, as apps/game carries whole tile
     * sizes rather than a scale factor -- and never a side effect of
     * having drawn some text.
     *
     * There is no shaping, no line breaking and no kerning anywhere
     * here, for antwika::font's reason: none of the three is a coverage
     * mask, and a caller that needs them wants a layout engine rather
     * than one more argument.  A byte of the text is one codepoint,
     * which is exact for ASCII and Latin-1 and is deliberately not
     * UTF-8 decoding.
     *
     * @param atlas The atlas the text would be drawn from.
     * @param text The characters that would be drawn; one the atlas
     * does not hold measures nothing, since an atlas holds no metrics
     * for a glyph it was never asked to pack.
     * @return The width the advances sum to and the height of one line,
     * with a negative total reported as zero; zero for empty text.
     */
    [[nodiscard]] Size atlasTextSize(
        const font::GlyphAtlas &atlas, std::string_view text) noexcept;

    /**
     * @brief Work out where each glyph of a line of text goes.
     *
     * The pen starts at origin and walks along the baseline, which sits
     * the atlas's own ascent below the top of the line -- so origin is
     * the top-left corner of the line box, which is what
     * IRenderer::drawText() means by one too.
     *
     * A character the atlas does not hold moves the pen nowhere and
     * draws nothing, and a character with no rectangle -- a space, or
     * anything else whose mask came out empty -- moves the pen and
     * draws nothing.  Both are ordinary answers rather than errors,
     * following antwika::font's rule that anything running while a
     * frame is drawn answers rather than fails.
     *
     * Every blit that comes back has passed blitIsDrawable() against
     * the atlas's own mask, so a caller cannot be handed one that two
     * backends would draw differently.
     *
     * @param atlas The atlas to take glyphs from.
     * @param origin Top-left corner of the line box.
     * @param text The characters to place.
     * @return One blit per drawable character, in reading order.
     */
    [[nodiscard]] std::vector<GlyphBlit> atlasTextBlits(
        const font::GlyphAtlas &atlas,
        Point origin,
        std::string_view text);

    /**
     * @brief Draw a line of text from an uploaded atlas texture.
     *
     * One drawTexture() per drawable character, which is what the whole
     * arrangement is for: a mask is uploaded once, and every colour of
     * text after that is a tint.
     * @param renderer The renderer to draw through.
     * @param texture The atlas, uploaded by that same renderer; one
     * from another renderer draws nothing, as it does anywhere else.
     * @param atlas The atlas the texture was made from.
     * @param origin Top-left corner of the line box.
     * @param text The characters to draw.
     * @param tint Multiplied into the mask, so this is what picks the
     * colour of the text.
     */
    void drawAtlasText(
        IRenderer &renderer,
        const ITexture &texture,
        const font::GlyphAtlas &atlas,
        Point origin,
        std::string_view text,
        Color tint);

} // namespace antwika::gfx
