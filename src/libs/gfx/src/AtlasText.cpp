#include "antwika/gfx/AtlasText.hpp"

#include <cstdint>
#include <string_view>
#include <vector>

#include <antwika/font/GlyphAtlas.hpp>
#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/RectF.hpp>

#include "antwika/gfx/Blit.hpp"
#include "antwika/gfx/Color.hpp"
#include "antwika/gfx/GlyphBlit.hpp"
#include "antwika/gfx/IRenderer.hpp"
#include "antwika/gfx/ITexture.hpp"
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

        [[nodiscard]] std::uint32_t clampToPixels(
            std::int64_t value) noexcept
        {
            if (value < 0)
            {
                return 0;
            }

            return static_cast<std::uint32_t>(value);
        }

        [[nodiscard]] Rect sourceRectOf(
            const font::AtlasGlyph &glyph) noexcept
        {
            return Rect{
                .origin =
                    {.x = static_cast<std::int32_t>(glyph.source.x),
                     .y = static_cast<std::int32_t>(glyph.source.y)},
                .size = {
                    .width = glyph.source.width,
                    .height = glyph.source.height}};
        }
    }

    Size atlasTextSize(
        const font::GlyphAtlas &atlas, std::string_view text) noexcept
    {
        if (text.empty())
        {
            return Size{};
        }

        std::int64_t width = 0;

        for (const char character : text)
        {
            const font::AtlasGlyph *glyph
                = atlas.find(codepointOf(character));

            if (glyph == nullptr)
            {
                continue;
            }

            width += glyph->metrics.advance;
        }

        return Size{
            .width = clampToPixels(width),
            .height = clampToPixels(atlas.metrics.lineHeight)};
    }

    std::vector<GlyphBlit> atlasTextBlits(
        const font::GlyphAtlas &atlas,
        Point origin,
        std::string_view text)
    {
        const Size mask{
            .width = atlas.coverage.width,
            .height = atlas.coverage.height};
        const std::int64_t baseline
            = static_cast<std::int64_t>(origin.y) + atlas.metrics.ascent;

        std::vector<GlyphBlit> blits;
        std::int64_t pen = origin.x;

        for (const char character : text)
        {
            const font::AtlasGlyph *glyph
                = atlas.find(codepointOf(character));

            if (glyph == nullptr)
            {
                continue;
            }

            const Rect source = sourceRectOf(*glyph);
            const GlyphBlit blit{
                .source = source,
                .destination = {
                    .origin =
                        {.x = static_cast<std::int32_t>(
                             pen + glyph->metrics.bearingX),
                         .y = static_cast<std::int32_t>(
                             baseline + glyph->metrics.bearingY)},
                    .size = source.size}};

            pen += glyph->metrics.advance;

            if (!blitIsDrawable(mask, blit.source, blit.destination))
            {
                continue;
            }

            blits.push_back(blit);
        }

        return blits;
    } // GCOVR_EXCL_LINE

    void drawAtlasText(
        IRenderer &renderer,
        const ITexture &texture,
        const font::GlyphAtlas &atlas,
        PointF origin,
        std::string_view text,
        Color tint)
    {
        for (const GlyphBlit &blit : atlasTextBlits(atlas, Point{}, text))
        {
            const RectF destination{
                PointF{
                    origin.x
                        + static_cast<float>(blit.destination.origin.x),
                    origin.y
                        + static_cast<float>(blit.destination.origin.y)},
                blit.destination.size};

            renderer.drawTexture(
                texture, blit.source, destination, tint);
        }
    }

}
