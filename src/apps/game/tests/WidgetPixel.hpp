#pragma once

#include <optional>

#include <antwika/gfx/Point.hpp>
#include <antwika/ui/Frame.hpp>
#include <antwika/ui/WidgetId.hpp>

namespace antwika::game::tests
{

    /**
     * @brief Find the pixel at the centre of a widget.
     * @param frame The frame the widget was declared in.
     * @param id The widget to look for.
     * @return Its centre, or nothing when the frame declared no such id.
     *
     * Where a widget is, is the layout's business, so a test asks the
     * layout rather than deciding for itself.
     *
     * These helpers used to sweep every other pixel of the canvas and
     * run a complete `describe()` at each one until the hovered id
     * matched -- up to 163,840 layouts to answer one question, which
     * was 48 seconds of `SaveLoadSinkTest` alone under the coverage
     * build's `-O0`.
     * `ui::Frame::rects` reports the same answer off the layout the
     * frame was already built from, so one `describe()` does it.
     *
     * The centre rather than a corner, because a rectangle's corner
     * belongs to it and to whatever is drawn against it, and the point
     * of asking is to name a pixel that hits this widget and no other.
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

} // namespace antwika::game::tests
