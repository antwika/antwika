#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "antwika/gfx/Color.hpp"
#include "antwika/gfx/GlyphCells.hpp"
#include "antwika/gfx/GlyphCellsCache.hpp"
#include "antwika/gfx/Point.hpp"
#include "antwika/gfx/Rect.hpp"
#include "antwika/gfx/Size.hpp"

namespace antwika::gfx
{

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

    template <typename Visit>
    void forEachGlyphPixel(
        GlyphCellsCache &cache,
        Point originPoint,
        std::string_view text,
        std::uint32_t scale,
        Color color,
        Visit visit)
    {
        if (scale == 0)
        {
            return;
        }

        const GlyphCells &cells = cache.at(scale);
        const Size size = cells.cellSize();
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
                        Rect{
                            .originPoint =
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

}
