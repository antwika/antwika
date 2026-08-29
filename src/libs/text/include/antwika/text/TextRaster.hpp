#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "antwika/gfx/Color.hpp"
#include "antwika/gfx/Glyphs.hpp"
#include "antwika/text/GlyphCells.hpp"
#include "antwika/text/GlyphCellsCache.hpp"
#include "antwika/gfx/Point.hpp"
#include "antwika/gfx/Rect.hpp"
#include "antwika/gfx/Size.hpp"

namespace antwika::text
{

    [[nodiscard]] constexpr gfx::Color getGlyphPixelColor(
        gfx::Color color, std::uint8_t coverage) noexcept
    {
        return gfx::Color{
            .red = color.red,
            .green = color.green,
            .blue = color.blue,
            .alpha = static_cast<std::uint8_t>(
                (static_cast<std::uint32_t>(color.alpha) * coverage)
                / 255)};
    }

    template <typename Visit>
    void forEachGlyphPixel(
        GlyphCellsCache &cache,
        gfx::Point originPoint,
        std::string_view text,
        gfx::TextScale scale,
        gfx::Color color,
        Visit visit)
    {
        if (scale.multiplier == 0)
        {
            return;
        }

        const GlyphCells &cells = cache.at(scale);
        const gfx::Size size = cells.getCellSize();
        const auto step = static_cast<std::int64_t>(size.width);
        const auto top = static_cast<std::int64_t>(originPoint.y);

        for (std::size_t cell = 0; cell < text.size(); ++cell)
        {
            const auto left = static_cast<std::int64_t>(originPoint.x)
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
                        gfx::Rect{
                            .originPoint =
                                {.x = static_cast<std::int32_t>(
                                     left + column),
                                 .y = static_cast<std::int32_t>(
                                     top + row)},
                            .size = {.width = 1, .height = 1}},
                        getGlyphPixelColor(color, coverage));
                }
            }
        }
    }

}
