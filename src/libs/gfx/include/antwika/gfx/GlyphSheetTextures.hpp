#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string_view>

#include <antwika/gfx/PointF.hpp>

#include "antwika/gfx/Color.hpp"
#include "antwika/gfx/GlyphCells.hpp"
#include "antwika/gfx/IRenderer.hpp"
#include "antwika/gfx/ITexture.hpp"
#include "antwika/gfx/PointF.hpp"

namespace antwika::gfx
{

    class GlyphSheetTextures final
    {
    public:
        GlyphSheetTextures() = default;

        GlyphSheetTextures(const GlyphSheetTextures &) = delete;
        GlyphSheetTextures(GlyphSheetTextures &&) = delete;

        GlyphSheetTextures &operator=(const GlyphSheetTextures &) = delete;
        GlyphSheetTextures &operator=(GlyphSheetTextures &&) = delete;

        void draw(
            IRenderer &renderer,
            PointF origin,
            std::string_view text,
            std::uint32_t scale,
            Color color);

    private:
        GlyphCellsCache cells;
        std::map<std::uint32_t, std::unique_ptr<ITexture>> sheets;
    };

}
