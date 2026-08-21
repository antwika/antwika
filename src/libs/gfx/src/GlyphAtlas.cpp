#include "antwika/gfx/GlyphAtlas.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

#include "antwika/gfx/Bitmap.hpp"
#include "antwika/gfx/GlyphBlit.hpp"
#include "antwika/gfx/GlyphCells.hpp"
#include "antwika/gfx/Glyphs.hpp"
#include "antwika/gfx/Point.hpp"
#include "antwika/gfx/Rect.hpp"
#include "antwika/gfx/Size.hpp"

namespace antwika::gfx
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

            if (codepoint < kFirstGlyph || codepoint > kLastGlyph)
            {
                return std::nullopt;
            }

            return static_cast<std::size_t>(codepoint - kFirstGlyph);
        }
    }

    Bitmap glyphAtlasBitmap(const GlyphCells &cells)
    {
        const Size cellSize = cells.cellSize();

        const Size sheetSize{
            .width = static_cast<std::uint32_t>(kGlyphCount) * cellSize.width,
            .height = cellSize.height};

        Bitmap bitmap{.size = sheetSize, .pixels = {}};
        bitmap.pixels.assign(
            static_cast<std::size_t>(sheetSize.width) * sheetSize.height
                * kBytesPerPixel,
            0);

        for (std::size_t index = 0; index < kGlyphCount; ++index)
        {
            const auto character = static_cast<char>(
                static_cast<unsigned char>(kFirstGlyph + index));

            for (std::uint32_t row = 0; row < cellSize.height; ++row)
            {
                for (std::uint32_t column = 0; column < cellSize.width;
                     ++column)
                {
                    const auto byteIndex =
                        ((static_cast<std::size_t>(row) * sheetSize.width)
                         + (index * cellSize.width) + column)
                        * kBytesPerPixel;

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

    std::optional<Rect> glyphAtlasCell(
        const GlyphCells &cells, char character) noexcept
    {
        const auto index = indexOf(character);

        if (!index.has_value())
        {
            return std::nullopt;
        }

        const Size cellSize = cells.cellSize();

        return Rect{
            .originPoint =
                {.x = static_cast<std::int32_t>(*index * cellSize.width),
                 .y = 0},
            .size = cellSize};
    }

    std::vector<GlyphBlit> glyphAtlasBlits(
        const GlyphCells &cells, Point originPoint, std::string_view text)
    {
        const Size cellSize = cells.cellSize();

        std::vector<GlyphBlit> blits;
        blits.reserve(text.size());

        for (std::size_t charIndex = 0; charIndex < text.size(); ++charIndex)
        {
            const auto source = glyphAtlasCell(cells, text[charIndex]);

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
