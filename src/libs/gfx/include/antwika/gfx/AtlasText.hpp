#pragma once

#include <string_view>
#include <vector>

#include <antwika/font/GlyphAtlas.hpp>

#include "antwika/gfx/Color.hpp"
#include "antwika/gfx/GlyphBlit.hpp"
#include "antwika/gfx/IRenderer.hpp"
#include "antwika/gfx/ITexture.hpp"
#include "antwika/gfx/Point.hpp"
#include "antwika/gfx/PointF.hpp"
#include "antwika/gfx/Size.hpp"

namespace antwika::gfx
{

    [[nodiscard]] Size atlasTextSize(
        const font::GlyphAtlas &atlas, std::string_view text) noexcept;

    [[nodiscard]] std::vector<GlyphBlit> atlasTextBlits(
        const font::GlyphAtlas &atlas,
        Point origin,
        std::string_view text);

    void drawAtlasText(
        IRenderer &renderer,
        const ITexture &texture,
        const font::GlyphAtlas &atlas,
        PointF origin,
        std::string_view text,
        Color tint);

}
