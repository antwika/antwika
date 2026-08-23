#pragma once

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

#include "antwika/gfx/Bitmap.hpp"
#include "antwika/text/GlyphBlit.hpp"
#include "antwika/text/GlyphCells.hpp"
#include "antwika/gfx/Point.hpp"
#include "antwika/gfx/Rect.hpp"

namespace antwika::text
{

    [[nodiscard]] gfx::Bitmap getGlyphAtlasBitmap(const GlyphCells &cells);

    [[nodiscard]] std::optional<gfx::Rect> getGlyphAtlasCell(
        const GlyphCells &cells, char character) noexcept;

    [[nodiscard]] std::vector<GlyphBlit> getGlyphAtlasBlits(
        const GlyphCells &cells, gfx::Point originPoint, std::string_view text);

}
