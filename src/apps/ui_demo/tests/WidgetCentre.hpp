#pragma once

#include <cstdint>
#include <optional>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/ui/Frame.hpp>
#include <antwika/ui/WidgetId.hpp>

namespace antwika::ui_demo::tests
{

    /**
     * @brief Find the pixel at the centre of a widget.
     *
     * Where a widget is, is the layout's business, so a test asks the
     * layout rather than deciding for itself -- which is exactly what
     * ui::Frame::rects is for, and one describe() answers it.
     *
     * The centre rather than a corner, because a rectangle's corner
     * belongs to it and to whatever is drawn against it, and the point
     * of asking is to name a pixel that hits this widget and no other.
     *
     * @param frame The frame the widget was declared in.
     * @param id The widget to look for.
     * @return Its centre, or nothing when the frame declared no such id.
     */
    [[nodiscard]] inline std::optional<gfx::Point> widgetCentre(
        const ui::Frame &frame, ui::WidgetId id)
    {
        const std::optional<gfx::Rect> rect = frame.rects.find(id);

        if (!rect.has_value())
        {
            return std::nullopt;
        }

        return gfx::Point{
            .x = rect->origin.x
                 + static_cast<std::int32_t>(rect->size.width) / 2,
            .y = rect->origin.y
                 + static_cast<std::int32_t>(rect->size.height) / 2};
    }

    /**
     * @brief Get the id of one of a dropdown's options.
     * @param base The id the first option carries.
     * @param index Which option to name.
     * @return That option's id.
     */
    [[nodiscard]] inline ui::WidgetId optionWidget(
        ui::WidgetId base, std::uint64_t index)
    {
        return static_cast<ui::WidgetId>(
            static_cast<std::uint64_t>(base) + index);
    }

} // namespace antwika::ui_demo::tests
