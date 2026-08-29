#pragma once

#include <cstddef>
#include <optional>

#include <antwika/gfx/PointF.hpp>
#include <antwika/geometry/Grid.hpp>
#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/gfx/Size.hpp>

namespace antwika::editor
{

    [[nodiscard]] gfx::RectF getCharacterSheetBounds(gfx::Size canvasSize);

    [[nodiscard]] gfx::RectF getCharacterDrawBounds(
        gfx::Size canvasSize, float railWidth);

    [[nodiscard]] gfx::RectF getCharacterPlace(
        gfx::RectF sheetRect, std::size_t direction, std::size_t frame);

    [[nodiscard]] std::optional<std::size_t> characterAt(
        gfx::RectF sheetRect, gfx::PointF point);

    [[nodiscard]] gfx::RectF getCharacterCanvasRect(gfx::RectF drawRect);

}
