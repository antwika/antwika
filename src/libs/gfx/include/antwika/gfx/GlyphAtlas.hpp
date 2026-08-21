#pragma once

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

#include "antwika/gfx/Bitmap.hpp"
#include "antwika/gfx/GlyphBlit.hpp"
#include "antwika/gfx/GlyphCells.hpp"
#include "antwika/gfx/Point.hpp"
#include "antwika/gfx/Rect.hpp"

namespace antwika::gfx
{

    [[nodiscard]] Bitmap glyphAtlasBitmap(const GlyphCells &cells);

    [[nodiscard]] std::optional<Rect> glyphAtlasCell(
        const GlyphCells &cells, char character) noexcept;

    [[nodiscard]] std::vector<GlyphBlit> glyphAtlasBlits(
        const GlyphCells &cells, Point originPoint, std::string_view text);

}
