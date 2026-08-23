#include "antwika/font/GlyphAtlas.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "antwika/font/Coverage.hpp"
#include "antwika/font/Font.hpp"
#include "antwika/font/Glyph.hpp"
#include "antwika/font/Rect.hpp"
#include "antwika/font/FontError.hpp"

namespace antwika::font
{

    namespace
    {
        [[nodiscard]] std::vector<char32_t> getDistinctAscending(
            std::span<const char32_t> codepoints)
        {
            std::vector<char32_t> codepointList(
                codepoints.begin(), codepoints.end());

            std::ranges::sort(codepointList);

            const auto duplicates = std::ranges::unique(codepointList);
            codepointList.erase(duplicates.begin(), duplicates.end());

            return codepointList;
        } // GCOVR_EXCL_LINE

        void blit(
            const Coverage &maskCoverage,
            const Rect &intoRect,
            Coverage &atlasCoverage)
        {
            for (std::uint32_t y = 0; y < maskCoverage.height; ++y)
            {
                for (std::uint32_t x = 0; x < maskCoverage.width; ++x)
                {
                    const std::size_t fromIndex
                        = static_cast<std::size_t>(y) * maskCoverage.width + x;
                    const std::size_t toIndex
                        = static_cast<std::size_t>(intoRect.y + y)
                            * atlasCoverage.width
                        + intoRect.x + x;

                    atlasCoverage.samples[toIndex] =
                        maskCoverage.samples[fromIndex];
                }
            }
        }

        class Shelves final
        {
        public:
            explicit Shelves(GlyphAtlas::Options options)
                : options(options),
                  penX(options.padding),
                  penY(options.padding)
            {
            }

            [[nodiscard]] Rect place(std::uint32_t width,
                std::uint32_t height)
            {
                const std::uint64_t row = std::uint64_t{width}
                    + 2 * std::uint64_t{options.padding};

                if (row > options.maxWidth)
                {
                    throw FontError(
                        "font: a glyph " + std::to_string(width)
                        + " pixels wide does not fit an atlas "
                        + std::to_string(options.maxWidth)
                        + " pixels wide");
                }

                if (std::uint64_t{penX} + width + options.padding
                    > options.maxWidth)
                {
                    penX = options.padding;
                    penY += shelfHeight + options.padding;
                    shelfHeight = 0;
                }

                const Rect placedRect{penX, penY, width, height};

                penX += width + options.padding;
                shelfHeight = std::max(shelfHeight, height);
                usedWidth = std::max(usedWidth, penX);
                usedHeight = penY + shelfHeight + options.padding;

                return placedRect;
            }

            [[nodiscard]] std::uint32_t getWidth() const
            {
                return usedWidth;
            }

            [[nodiscard]] std::uint32_t getHeight() const
            {
                return usedHeight;
            }

        private:
            GlyphAtlas::Options options;
            std::uint32_t penX = 0;
            std::uint32_t penY = 0;
            std::uint32_t shelfHeight = 0;
            std::uint32_t usedWidth = 0;
            std::uint32_t usedHeight = 0;
        };
    }

    const AtlasGlyph *GlyphAtlas::getFind(char32_t codepoint) const
    {
        const auto foundGlyph = std::ranges::lower_bound(
            glyphs, codepoint, {}, &AtlasGlyph::codepoint);

        if (foundGlyph == glyphs.end() || foundGlyph->codepoint != codepoint)
        {
            return nullptr;
        }

        return &*foundGlyph;
    }

    GlyphAtlas createGlyphAtlas(
        const Font &font,
        std::span<const char32_t> codepoints,
        std::uint32_t pixelHeight,
        GlyphAtlas::Options options)
    {
        if (codepoints.empty())
        {
            throw FontError(
                "font: an atlas of no characters is not one anything can "
                "draw from");
        }

        GlyphAtlas atlas;
        atlas.metrics = font.getMetrics(pixelHeight);

        std::vector<Coverage> coverageMasks;
        Shelves shelves{options};

        for (const char32_t codepoint : getDistinctAscending(codepoints))
        {
            Glyph glyph = font.getRasterise(codepoint, pixelHeight);
            Rect sourceRect;

            if (!glyph.coverage.samples.empty())
            {
                sourceRect = shelves.place(
                    glyph.coverage.width, glyph.coverage.height);
            }

            atlas.glyphs.push_back(AtlasGlyph{
                .codepoint = codepoint,
                .sourceRect = sourceRect,
                .metrics = glyph.metrics});

            coverageMasks.push_back(std::move(glyph.coverage));
        }

        atlas.coverage.width = shelves.getWidth();
        atlas.coverage.height = shelves.getHeight();
        atlas.coverage.samples.assign(
            static_cast<std::size_t>(shelves.getWidth()) * shelves.getHeight(),
            0);

        for (std::size_t index = 0; index < coverageMasks.size(); ++index)
        {
            blit(
                coverageMasks[index],
                atlas.glyphs[index].sourceRect,
                atlas.coverage);
        }

        return atlas;
    }

}
