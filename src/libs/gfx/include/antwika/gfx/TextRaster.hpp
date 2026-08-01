#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "antwika/gfx/Color.hpp"
#include "antwika/gfx/GlyphCells.hpp"
#include "antwika/gfx/Point.hpp"
#include "antwika/gfx/Rect.hpp"
#include "antwika/gfx/Size.hpp"

namespace antwika::gfx
{

    /**
     * @brief Work out what colour one pixel of a glyph is drawn in.
     *
     * The whole of the anti-aliasing rule, in one place because it is
     * the sort of arithmetic two backends would each get slightly
     * right.  Coverage multiplies the colour's own alpha rather than
     * replacing it, so drawing text in a translucent colour stays
     * translucent, and integer division makes the answer the same
     * everywhere rather than nearly the same.
     * @param color The colour the text is drawn in.
     * @param coverage How much ink landed on the pixel, 0 to 255.
     * @return The colour, with its alpha scaled by the coverage.
     */
    [[nodiscard]] constexpr Color glyphPixelColor(
        Color color, std::uint8_t coverage) noexcept
    {
        return Color{
            .red = color.red,
            .green = color.green,
            .blue = color.blue,
            .alpha = static_cast<std::uint8_t>(
                (static_cast<std::uint32_t>(color.alpha) * coverage)
                / 255)};
    }

    /**
     * @brief Walk the inked pixels of a line of text, in drawing order.
     *
     * The one place the built-in font is turned into rectangles.
     * IRenderer::drawText() promises that every backend draws the same
     * glyphs at the metrics textSize() reports, and no backend has a
     * font of its own, so each of them paints what this hands it -- and
     * doing that arithmetic once per backend is exactly how two of them
     * come to draw two different pictures.
     * textSize() is the other half of the same promise, and it already
     * lives in one place for the same reason.
     *
     * The ink comes from GlyphCells, which is a real font rasterised
     * onto the fixed cell grid textSize() measures, so no lit pixel
     * ever leaves the box that call reports.  The colour comes back
     * ready to fill with, coverage folded into its alpha, which is why
     * the colour is an argument here: a backend that had to fold it in
     * itself would be a backend that could fold it in differently.
     *
     * Nothing is allocated and nothing is drawn: each inked pixel is
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
     * inks no pixel, and so occupies a blank cell of the same width.
     * @param scale Pixels per glyph pixel; zero visits nothing, which
     * is what IRenderer::drawText() says a zero scale draws.
     * @param color The colour the text is drawn in.
     * @param visit Called with the Rect of each inked pixel and the
     * colour to fill it with.
     */
    template <typename Visit>
    void forEachGlyphPixel(
        Point origin,
        std::string_view text,
        std::uint32_t scale,
        Color color,
        Visit visit)
    {
        if (scale == 0)
        {
            return;
        }

        const GlyphCells &cells = glyphCells(scale);
        const Size size = cells.cellSize();
        const auto step = static_cast<std::int64_t>(size.width);
        const auto top = static_cast<std::int64_t>(origin.y);

        for (std::size_t cell = 0; cell < text.size(); ++cell)
        {
            const auto left = static_cast<std::int64_t>(origin.x)
                + (static_cast<std::int64_t>(cell) * step);

            for (std::uint32_t row = 0; row < size.height; ++row)
            {
                for (std::uint32_t column = 0; column < size.width;
                     ++column)
                {
                    const auto coverage =
                        cells.coverageAt(text[cell], column, row);

                    if (coverage == 0)
                    {
                        continue;
                    }

                    visit(
                        Rect{
                            .origin =
                                {.x = static_cast<std::int32_t>(
                                     left + column),
                                 .y = static_cast<std::int32_t>(
                                     top + row)},
                            .size = {.width = 1, .height = 1}},
                        glyphPixelColor(color, coverage));
                }
            }
        }
    }

} // namespace antwika::gfx
