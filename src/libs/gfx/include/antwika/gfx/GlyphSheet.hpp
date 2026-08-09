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

    [[nodiscard]] Bitmap glyphSheetBitmap(const GlyphCells &cells);

    [[nodiscard]] std::optional<Rect> glyphSheetCell(
        const GlyphCells &cells, char character) noexcept;

    [[nodiscard]] std::vector<GlyphBlit> glyphSheetBlits(
        const GlyphCells &cells, Point origin, std::string_view text);

}
