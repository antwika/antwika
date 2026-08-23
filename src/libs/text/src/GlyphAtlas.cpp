#include "antwika/text/GlyphAtlas.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

#include "antwika/gfx/Bitmap.hpp"
#include "antwika/text/GlyphBlit.hpp"
#include "antwika/text/GlyphCells.hpp"
#include "antwika/gfx/Glyphs.hpp"
#include "antwika/gfx/Point.hpp"
#include "antwika/gfx/Rect.hpp"
#include "antwika/gfx/Size.hpp"

namespace antwika::text
{

    namespace
    {
        [[nodiscard]] char32_t codepointOf(char character) noexcept
        {
            return static_cast<char32_t>(
                static_cast<unsigned char>(character));
        }

        [[nodiscard]] std::optional<std::size_t> indexOf(
            char character) noexcept
        {
            const auto codepoint = codepointOf(character);

            if (codepoint < gfx::kFirstGlyph || codepoint > gfx::kLastGlyph)
            {
                return std::nullopt;
            }

            return static_cast<std::size_t>(codepoint - gfx::kFirstGlyph);
        }
    }

    gfx::Bitmap getGlyphAtlasBitmap(const GlyphCells &cells)
    {
        const gfx::Size cellSize = cells.getCellSize();

        const gfx::Size sheetSize{
            .width = static_cast<std::uint32_t>(gfx::kGlyphCount)
                     * cellSize.width,
            .height = cellSize.height};

        gfx::Bitmap bitmap{.size = sheetSize, .pixels = {}};
        bitmap.pixels.assign(
            static_cast<std::size_t>(sheetSize.width) * sheetSize.height
                * gfx::kBytesPerPixel,
            0);

        for (std::size_t index = 0; index < gfx::kGlyphCount; ++index)
        {
            const auto character = static_cast<char>(
                static_cast<unsigned char>(gfx::kFirstGlyph + index));

            for (std::uint32_t row = 0; row < cellSize.height; ++row)
            {
                for (std::uint32_t column = 0; column < cellSize.width;
                     ++column)
                {
                    const auto byteIndex =
                        ((static_cast<std::size_t>(row) * sheetSize.width)
                         + (index * cellSize.width) + column)
                        * gfx::kBytesPerPixel;

                    bitmap.pixels[byteIndex] = 255;
                    bitmap.pixels[byteIndex + 1] = 255;
                    bitmap.pixels[byteIndex + 2] = 255;
                    bitmap.pixels[byteIndex + 3] =
                        cells.coverageAt(character, column, row);
                }
            }
        }

        return bitmap;
    } // GCOVR_EXCL_LINE

    std::optional<gfx::Rect> getGlyphAtlasCell(
        const GlyphCells &cells, char character) noexcept
    {
        const auto index = indexOf(character);

        if (!index.has_value())
        {
            return std::nullopt;
        }

        const gfx::Size cellSize = cells.getCellSize();

        return gfx::Rect{
            .originPoint =
                {.x = static_cast<std::int32_t>(*index * cellSize.width),
                 .y = 0},
            .size = cellSize};
    }

    std::vector<GlyphBlit> getGlyphAtlasBlits(
        const GlyphCells &cells, gfx::Point originPoint, std::string_view text)
    {
        const gfx::Size cellSize = cells.getCellSize();

        std::vector<GlyphBlit> blits;
        blits.reserve(text.size());

        for (std::size_t charIndex = 0; charIndex < text.size(); ++charIndex)
        {
            const auto source = getGlyphAtlasCell(cells, text[charIndex]);

            if (!source.has_value())
            {
                continue;
            }

            blits.push_back(
                GlyphBlit{ // GCOVR_EXCL_LINE
                    .sourceRect = *source,
                    .destinationRect = {
                        .originPoint =
                            {.x = originPoint.x
                                 + static_cast<std::int32_t>(
                                     charIndex * cellSize.width),
                             .y = originPoint.y},
                        .size = cellSize}});
        }

        return blits;
    } // GCOVR_EXCL_LINE

}
