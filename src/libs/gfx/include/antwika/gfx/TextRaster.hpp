#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "antwika/gfx/Glyphs.hpp"
#include "antwika/gfx/Point.hpp"
#include "antwika/gfx/Rect.hpp"

namespace antwika::gfx
{

    /**
     * @brief Walk the lit pixels of a line of text, in drawing order.
     *
     * The one place the built-in font is turned into rectangles.
     * IRenderer::drawText() promises that every backend draws the same
     * glyphs at the metrics textSize() reports, and no backend has a
     * font of its own, so each of them paints glyphRow() as filled
     * rectangles -- and doing that arithmetic once per backend is
     * exactly how two of them come to draw two different pictures.
     * textSize() is the other half of the same promise, and it already
     * lives in one place for the same reason.
     *
     * Nothing is allocated and nothing is drawn: each lit pixel is
     * handed to the visitor as it is worked out, left to right and top
     * to bottom within a cell, and cell by cell along the line.
     * A backend turns that Rect into whatever its framework fills.
     *
     * Positions are worked out 64 bits wide, since an int32 origin plus
     * a glyph offset is a sum that can wrap, and a wrapped glyph lands
     * somewhere absurd rather than merely off canvas.
     *
     * @param origin Top-left corner of the first glyph's cell.
     * @param text The characters to walk; one the font has no glyph for
     * lights no pixel, and so occupies a blank cell of the same width.
     * @param scale Pixels per glyph pixel; zero visits nothing, which
     * is what IRenderer::drawText() says a zero scale draws.
     * @param visit Called with the Rect of each lit pixel.
     */
    template <typename Visit>
    void forEachGlyphPixel(
        Point origin,
        std::string_view text,
        std::uint32_t scale,
        Visit visit)
    {
        if (scale == 0)
        {
            return;
        }

        const auto step = static_cast<std::int64_t>(scale);
        const auto top = static_cast<std::int64_t>(origin.y);

        for (std::size_t cell = 0; cell < text.size(); ++cell)
        {
            const auto left =
                static_cast<std::int64_t>(origin.x)
                + (static_cast<std::int64_t>(cell) * kGlyphAdvance
                   * step);

            for (std::uint32_t row = 0; row < kGlyphHeight; ++row)
            {
                const auto bits = glyphRow(text[cell], row);

                for (std::uint32_t column = 0; column < kGlyphWidth;
                     ++column)
                {
                    const auto shift = kGlyphWidth - 1 - column;

                    if (((bits >> shift) & 1U) == 0)
                    {
                        continue;
                    }

                    visit(Rect{
                        .origin =
                            {.x = static_cast<std::int32_t>(
                                 left + (column * step)),
                             .y = static_cast<std::int32_t>(
                                 top + (row * step))},
                        .size = {.width = scale, .height = scale}});
                }
            }
        }
    }

} // namespace antwika::gfx
