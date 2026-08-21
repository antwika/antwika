#include "antwika/gfx/GlyphCells.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

#include <antwika/font/GlyphAtlas.hpp>
#include <antwika/gfx/GlyphCellsCache.hpp>

#include "antwika/gfx/Glyphs.hpp"
#include "antwika/gfx/Size.hpp"

#include "BuiltInFont.hpp"

namespace antwika::gfx
{

    namespace
    {
        [[nodiscard]] std::vector<char32_t> coveredCodepoints()
        {
            std::vector<char32_t> codepoints;
            codepoints.reserve(kGlyphCount);

            for (char32_t codepoint = kFirstGlyph;
                 codepoint <= kLastGlyph;
                 ++codepoint)
            {
                codepoints.push_back(codepoint);
            }

            return codepoints;
        } // GCOVR_EXCL_LINE
    }

    GlyphCells::GlyphCells(std::uint32_t scale)
        : cell{
              .width = scaledGlyphAdvance(scale),
              .height = scaledGlyphLineHeight(scale)}
    {
        if (cell.height == 0)
        {
            return;
        }

        const font::GlyphAtlas atlas = font::makeGlyphAtlas(
            detail::builtInFont(), coveredCodepoints(), cell.height);

        samples.assign(kGlyphCount * cell.width * cell.height, 0);

        for (const font::AtlasGlyph &glyph : atlas.glyphs)
        {
            const auto index =
                static_cast<std::size_t>(glyph.codepoint - kFirstGlyph);
            const auto originX =
                static_cast<std::int64_t>(glyph.metrics.bearingX);
            const auto originY = static_cast<std::int64_t>(
                atlas.metrics.ascent + glyph.metrics.bearingY);

            for (std::uint32_t row = 0; row < cell.height; ++row)
            {
                for (std::uint32_t column = 0; column < cell.width;
                     ++column)
                {
                    const auto x = static_cast<std::uint32_t>(
                        static_cast<std::int64_t>(column) - originX);
                    const auto y = static_cast<std::uint32_t>(
                        static_cast<std::int64_t>(row) - originY);

                    if (x >= glyph.sourceRect.width
                        || y >= glyph.sourceRect.height)
                    {
                        continue;
                    }

                    const auto sampleIndex =
                        ((index * cell.height) + row) * cell.width
                        + column;

                    samples[sampleIndex] = atlas.coverage.at(
                        glyph.sourceRect.x + x, glyph.sourceRect.y + y);
                }
            }
        }
    }

    Size GlyphCells::cellSize() const noexcept
    {
        return cell;
    }

    std::uint8_t GlyphCells::coverageAt(
        char character,
        std::uint32_t column,
        std::uint32_t row) const noexcept
    {
        const auto codepoint =
            static_cast<char32_t>(static_cast<unsigned char>(character));

        if (codepoint < kFirstGlyph || codepoint > kLastGlyph)
        {
            return 0;
        }

        if (column >= cell.width || row >= cell.height)
        {
            return 0;
        }

        const auto index =
            static_cast<std::size_t>(codepoint - kFirstGlyph);

        return samples[((index * cell.height) + row) * cell.width
                       + column];
    }

    const GlyphCells &GlyphCellsCache::at(std::uint32_t scale)
    {
        const auto foundCells = cells.find(scale);

        if (foundCells != cells.end())
        {
            return foundCells->second;
        }

        return cells.emplace(scale, GlyphCells{scale}).first->second;
    }

}
