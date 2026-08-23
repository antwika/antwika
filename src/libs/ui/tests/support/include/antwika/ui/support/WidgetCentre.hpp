#pragma once

#include <cstdint>
#include <optional>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>

#include "antwika/ui/Frame.hpp"
#include "antwika/ui/WidgetId.hpp"

namespace antwika::ui::support
{

    [[nodiscard]] inline std::optional<gfx::Point> getWidgetCentre(
        const Frame &frame, const WidgetId widget)
    {
        const std::optional<gfx::Rect> rect = frame.rects.find(widget);

        if (!rect.has_value())
        {
            return std::nullopt;
        }

        return gfx::Point{
            .x = rect->originPoint.x
                 + static_cast<std::int32_t>(rect->size.width) / 2,
            .y = rect->originPoint.y
                 + static_cast<std::int32_t>(rect->size.height) / 2};
    }

}
