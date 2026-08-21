#include "antwika/gfx/GlyphAtlasTextures.hpp"

#include <cstdint>
#include <string_view>
#include <utility>

#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/RectF.hpp>

#include "antwika/gfx/Color.hpp"
#include "antwika/gfx/GlyphBlit.hpp"
#include "antwika/gfx/GlyphCells.hpp"
#include "antwika/gfx/GlyphAtlas.hpp"
#include "antwika/gfx/IRenderer.hpp"
#include "antwika/gfx/Point.hpp"
#include "antwika/gfx/PointF.hpp"
#include "antwika/gfx/RectF.hpp"

namespace antwika::gfx
{

    void GlyphAtlasTextures::draw(
        IRenderer &renderer,
        PointF originPoint,
        std::string_view text,
        std::uint32_t scale,
        Color color)
    {
        if (scale == 0 || text.empty())
        {
            return;
        }

        const GlyphCells &bakedCells = cells.at(scale);

        auto atlas = atlasTextures.find(scale);

        if (atlas == atlasTextures.end())
        {
            atlas = atlasTextures
                        .emplace(
                            scale,
                            renderer.createTexture(
                                glyphAtlasBitmap(bakedCells)))
                        .first;
        }

        if (atlas->second == nullptr)
        {
            return;
        }

        for (const GlyphBlit &blit : glyphAtlasBlits(bakedCells, Point{}, text))
        {
            const RectF destinationRect{
                antwika::gfx::PointF{
                    originPoint.x
                        + static_cast<float>(
                            blit.destinationRect.originPoint.x),
                    originPoint.y
                        + static_cast<float>(
                            blit.destinationRect.originPoint.y)},
                blit.destinationRect.size};

            renderer.drawTexture(
                *atlas->second, blit.sourceRect, destinationRect, color);
        }
    }

}
