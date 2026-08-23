#pragma once

#include <string_view>
#include <vector>

#include <antwika/font/GlyphAtlas.hpp>

#include "antwika/gfx/Color.hpp"
#include "antwika/text/GlyphBlit.hpp"
#include "antwika/gfx/IRenderer.hpp"
#include "antwika/gfx/ITexture.hpp"
#include "antwika/gfx/Point.hpp"
#include "antwika/gfx/PointF.hpp"
#include "antwika/gfx/Size.hpp"

namespace antwika::text
{

    [[nodiscard]] gfx::Size atlasTextSize(
        const font::GlyphAtlas &atlas, std::string_view text) noexcept;

    [[nodiscard]] std::vector<GlyphBlit> atlasTextBlits(
        const font::GlyphAtlas &atlas,
        gfx::Point originPoint,
        std::string_view text);

    void drawAtlasText(
        gfx::IRenderer &renderer,
        const gfx::ITexture &texture,
        const font::GlyphAtlas &atlas,
        gfx::PointF originPoint,
        std::string_view text,
        gfx::Color tintColor);

}
