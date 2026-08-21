#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string_view>

#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/GlyphCellsCache.hpp>

#include "antwika/gfx/Color.hpp"
#include "antwika/gfx/GlyphCells.hpp"
#include "antwika/gfx/IRenderer.hpp"
#include "antwika/gfx/ITexture.hpp"
#include "antwika/gfx/PointF.hpp"

namespace antwika::gfx
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
            IRenderer &renderer,
            PointF originPoint,
            std::string_view text,
            std::uint32_t scale,
            Color color);

    private:
        GlyphCellsCache cells;
        std::map<std::uint32_t, std::unique_ptr<ITexture>> atlasTextures;
    };

}
