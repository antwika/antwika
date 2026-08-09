#include "antwika/gfx/GlyphSheet.hpp"

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

    Bitmap glyphSheetBitmap(const GlyphCells &cells)
    {
        const Size cell = cells.cellSize();

        const Size sheet{
            .width = static_cast<std::uint32_t>(kGlyphCount) * cell.width,
            .height = cell.height};

        Bitmap bitmap{.size = sheet, .pixels = {}};
        bitmap.pixels.assign(
            static_cast<std::size_t>(sheet.width) * sheet.height
                * kBytesPerPixel,
            0);

        for (std::size_t index = 0; index < kGlyphCount; ++index)
        {
            const auto character = static_cast<char>(
                static_cast<unsigned char>(kFirstGlyph + index));

            for (std::uint32_t row = 0; row < cell.height; ++row)
            {
                for (std::uint32_t column = 0; column < cell.width;
                     ++column)
                {
                    const auto at =
                        ((static_cast<std::size_t>(row) * sheet.width)
                         + (index * cell.width) + column)
                        * kBytesPerPixel;

                    bitmap.pixels[at] = 255;
                    bitmap.pixels[at + 1] = 255;
                    bitmap.pixels[at + 2] = 255;
                    bitmap.pixels[at + 3] =
                        cells.coverageAt(character, column, row);
                }
            }
        }

        return bitmap;
    } // GCOVR_EXCL_LINE

    std::optional<Rect> glyphSheetCell(
        const GlyphCells &cells, char character) noexcept
    {
        const auto index = indexOf(character);

        if (!index.has_value())
        {
            return std::nullopt;
        }

        const Size cell = cells.cellSize();

        return Rect{
            .origin =
                {.x = static_cast<std::int32_t>(*index * cell.width),
                 .y = 0},
            .size = cell};
    }

    std::vector<GlyphBlit> glyphSheetBlits(
        const GlyphCells &cells, Point origin, std::string_view text)
    {
        const Size cell = cells.cellSize();

        std::vector<GlyphBlit> blits;
        blits.reserve(text.size());

        for (std::size_t at = 0; at < text.size(); ++at)
        {
            const auto source = glyphSheetCell(cells, text[at]);

            if (!source.has_value())
            {
                continue;
            }

            blits.push_back(
                GlyphBlit{ // GCOVR_EXCL_LINE
                    .source = *source,
                    .destination = {
                        .origin =
                            {.x = origin.x
                                 + static_cast<std::int32_t>(
                                     at * cell.width),
                             .y = origin.y},
                        .size = cell}});
        }

        return blits;
    } // GCOVR_EXCL_LINE

}
