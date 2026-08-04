#include "antwika/gfx/GlyphCells.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

#include <antwika/font/GlyphAtlas.hpp>

#include "antwika/gfx/Glyphs.hpp"
#include "antwika/gfx/Size.hpp"

#include "BuiltInFont.hpp"

namespace antwika::gfx
{

    namespace
    {
        // The range asked of the font, and the range a cell exists for.
        // Printable ASCII, exactly what the bitmap font it replaced had.
        constexpr char32_t kFirstGlyph = U' ';
        constexpr char32_t kLastGlyph = U'~';

        constexpr std::size_t kGlyphCount = kLastGlyph - kFirstGlyph + 1;

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
    } // namespace

    GlyphCells::GlyphCells(std::uint32_t scale)
        : cell{
              .width = kGlyphAdvance * scale,
              .height = kGlyphLineHeight * scale}
    {
        if (scale == 0)
        {
            return;
        }

        // The atlas is rasterised at the cell's own height.
        // So ascender-to-descender is the line textSize() reports.
        // The baseline sits that ascent below the cell's top.
        // Nothing here picks a size to suit the letters better.
        // A fitting rule invented here could disagree with textSize().
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
                    // A position outside the glyph goes negative.
                    // Unsigned, that wraps past every extent.
                    // So one test per axis rejects both sides.
                    // Any glyph smaller than a cell exercises both.
                    const auto x = static_cast<std::uint32_t>(
                        static_cast<std::int64_t>(column) - originX);
                    const auto y = static_cast<std::uint32_t>(
                        static_cast<std::int64_t>(row) - originY);

                    if (x >= glyph.source.width
                        || y >= glyph.source.height)
                    {
                        continue;
                    }

                    const auto at =
                        ((index * cell.height) + row) * cell.width
                        + column;

                    samples[at] = atlas.coverage.at(
                        glyph.source.x + x, glyph.source.y + y);
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
        // A byte is one codepoint, which is ASCII exactly.
        // Going through unsigned char is what keeps it that way.
        // A plain char is signed, and a high byte would sign-extend.
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
        const auto found = cells.find(scale);

        if (found != cells.end())
        {
            return found->second;
        }

        return cells.emplace(scale, GlyphCells{scale}).first->second;
    }

} // namespace antwika::gfx
