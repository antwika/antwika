#include "antwika/gfx/AtlasText.hpp"

#include <cstdint>
#include <string_view>
#include <vector>

#include <antwika/font/GlyphAtlas.hpp>

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
        // A byte is one codepoint, which is ASCII and Latin-1 exactly.
        // Going through unsigned char is what keeps it that way.
        // A plain char is signed, and a high byte would sign-extend.
        // The codepoint out of that is one no atlas ever holds.
        [[nodiscard]] char32_t codepointOf(char character) noexcept
        {
            return static_cast<char32_t>(
                static_cast<unsigned char>(character));
        }

        // Positions are worked out 64 bits wide, as TextRaster does.
        // An origin plus a run of advances is a sum that can wrap.
        // A wrapped glyph lands somewhere absurd rather than off view.
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
    } // namespace

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

            // A space keeps its advance and takes no room in a mask.
            // So it arrives here as a rectangle of nothing.
            // Asking the library's own rule answers that case.
            // It answers an atlas pointing outside its mask as well.
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
        Point origin,
        std::string_view text,
        Color tint)
    {
        for (const GlyphBlit &blit :
             atlasTextBlits(atlas, origin, text))
        {
            renderer.drawTexture(
                texture, blit.source, blit.destination, tint);
        }
    }

} // namespace antwika::gfx
