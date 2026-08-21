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
                .originPoint =
                    {.x = static_cast<std::int32_t>(glyph.sourceRect.x),
                     .y = static_cast<std::int32_t>(glyph.sourceRect.y)},
                .size = {
                    .width = glyph.sourceRect.width,
                    .height = glyph.sourceRect.height}};
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
        Point originPoint,
        std::string_view text)
    {
        const Size maskSize{
            .width = atlas.coverage.width,
            .height = atlas.coverage.height};
        const std::int64_t baseline
            = static_cast<std::int64_t>(originPoint.y) + atlas.metrics.ascent;

        std::vector<GlyphBlit> blits;
        std::int64_t pen = originPoint.x;

        for (const char character : text)
        {
            const font::AtlasGlyph *glyph
                = atlas.find(codepointOf(character));

            if (glyph == nullptr)
            {
                continue;
            }

            const Rect sourceRect = sourceRectOf(*glyph);
            const GlyphBlit blit{
                .sourceRect = sourceRect,
                .destinationRect = {
                    .originPoint =
                        {.x = static_cast<std::int32_t>(
                             pen + glyph->metrics.bearingX),
                         .y = static_cast<std::int32_t>(
                             baseline + glyph->metrics.bearingY)},
                    .size = sourceRect.size}};

            pen += glyph->metrics.advance;

            if (!blitIsInBounds(
                    maskSize, blit.sourceRect, blit.destinationRect))
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
        PointF originPoint,
        std::string_view text,
        Color tintColor)
    {
        for (const GlyphBlit &blit : atlasTextBlits(atlas, Point{}, text))
        {
            const RectF destinationRect{
                PointF{
                    originPoint.x
                        + static_cast<float>(
                            blit.destinationRect.originPoint.x),
                    originPoint.y
                        + static_cast<float>(
                            blit.destinationRect.originPoint.y)},
                blit.destinationRect.size};

            renderer.drawTexture(
                texture, blit.sourceRect, destinationRect, tintColor);
        }
    }

}
