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

    [[nodiscard]] gfx::RectF getCharacterPlace(
        gfx::Size canvasSize, std::size_t direction, std::size_t frame);

    [[nodiscard]] std::optional<std::size_t> characterAt(
        gfx::Size canvasSize, gfx::PointF point);

    [[nodiscard]] gfx::RectF getCharacterCanvasRect(gfx::Size canvasSize);

}
