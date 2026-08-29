#pragma once

#include <map>
#include <memory>
#include <string_view>

#include <antwika/gfx/PointF.hpp>
#include <antwika/text/GlyphCellsCache.hpp>

#include "antwika/gfx/Color.hpp"
#include "antwika/gfx/Glyphs.hpp"
#include "antwika/text/GlyphCells.hpp"
#include "antwika/gfx/IRenderer.hpp"
#include "antwika/gfx/ITexture.hpp"
#include "antwika/gfx/PointF.hpp"

namespace antwika::text
{

    class GlyphAtlasTextures final
    {
    public:
        GlyphAtlasTextures() = default;

        GlyphAtlasTextures(const GlyphAtlasTextures &) = delete;
        GlyphAtlasTextures(GlyphAtlasTextures &&) = delete;

        GlyphAtlasTextures &operator=(const GlyphAtlasTextures &) = delete;
        GlyphAtlasTextures &operator=(GlyphAtlasTextures &&) = delete;

        void draw(
            gfx::IRenderer &renderer,
            gfx::PointF originPoint,
            std::string_view text,
            gfx::TextScale scale,
            gfx::Color color);

    private:
        GlyphCellsCache cells;
        std::map<gfx::TextScale, std::unique_ptr<gfx::ITexture>>
            atlasTextures;
    };

}
