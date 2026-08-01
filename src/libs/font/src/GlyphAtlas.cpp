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
        [[nodiscard]] std::vector<char32_t> distinctAscending(
            std::span<const char32_t> codepoints)
        {
            std::vector<char32_t> wanted(
                codepoints.begin(), codepoints.end());

            std::ranges::sort(wanted);

            const auto duplicates = std::ranges::unique(wanted);
            wanted.erase(duplicates.begin(), duplicates.end());

            return wanted;
        } // GCOVR_EXCL_LINE

        void blit(
            const Coverage &mask, const Rect &into, Coverage &atlas)
        {
            for (std::uint32_t y = 0; y < mask.height; ++y)
            {
                for (std::uint32_t x = 0; x < mask.width; ++x)
                {
                    const std::size_t from
                        = static_cast<std::size_t>(y) * mask.width + x;
                    const std::size_t to
                        = static_cast<std::size_t>(into.y + y)
                            * atlas.width
                        + into.x + x;

                    atlas.samples[to] = mask.samples[from];
                }
            }
        }

        // Where the next glyph goes, and how big the mask has grown.
        // Glyphs run left to right along a shelf.
        // One that will not fit the row starts the next one.
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

                const Rect placed{penX, penY, width, height};

                penX += width + options.padding;
                shelfHeight = std::max(shelfHeight, height);
                usedWidth = std::max(usedWidth, penX);
                usedHeight = penY + shelfHeight + options.padding;

                return placed;
            }

            [[nodiscard]] std::uint32_t width() const
            {
                return usedWidth;
            }

            [[nodiscard]] std::uint32_t height() const
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
    } // namespace

    const AtlasGlyph *GlyphAtlas::find(char32_t codepoint) const
    {
        const auto found = std::ranges::lower_bound(
            glyphs, codepoint, {}, &AtlasGlyph::codepoint);

        if (found == glyphs.end() || found->codepoint != codepoint)
        {
            return nullptr;
        }

        return &*found;
    }

    GlyphAtlas makeGlyphAtlas(
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
        atlas.metrics = font.metrics(pixelHeight);

        std::vector<Coverage> masks;
        Shelves shelves{options};

        for (const char32_t codepoint : distinctAscending(codepoints))
        {
            Glyph glyph = font.rasterise(codepoint, pixelHeight);
            Rect source;

            // A space draws nothing and still moves the pen.
            // So it takes no room here and keeps its metrics.
            if (!glyph.coverage.samples.empty())
            {
                source = shelves.place(
                    glyph.coverage.width, glyph.coverage.height);
            }

            atlas.glyphs.push_back(AtlasGlyph{
                .codepoint = codepoint,
                .source = source,
                .metrics = glyph.metrics});

            masks.push_back(std::move(glyph.coverage));
        }

        atlas.coverage.width = shelves.width();
        atlas.coverage.height = shelves.height();
        atlas.coverage.samples.assign(
            static_cast<std::size_t>(shelves.width()) * shelves.height(),
            0);

        for (std::size_t index = 0; index < masks.size(); ++index)
        {
            blit(
                masks[index],
                atlas.glyphs[index].source,
                atlas.coverage);
        }

        return atlas;
    }

} // namespace antwika::font
