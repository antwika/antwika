#pragma once

#include <cmath>
#include <cstdint>

#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/gfx/Viewport.hpp>
#include <antwika/input/Position.hpp>

namespace antwika::editor::fakes
{

    [[nodiscard]] inline gfx::PointF getMiddleOf(const gfx::RectF whereRect)
    {
        return gfx::PointF{
            whereRect.originPoint.x + (whereRect.size.width / 2.0F),
            whereRect.originPoint.y + (whereRect.size.height / 2.0F)};
    }

    [[nodiscard]] inline input::Position getPointerPositionAt(
        const gfx::Viewport &viewport, const gfx::PointF canvasPoint)
    {
        const auto windowPoint = viewport.toWindow(canvasPoint);

        return input::Position{
            .x = static_cast<std::int32_t>(std::floor(windowPoint.x)),
            .y = static_cast<std::int32_t>(std::floor(windowPoint.y))};
    }

}
