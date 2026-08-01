#pragma once

#include <cstdint>
#include <optional>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/ui/Frame.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "antwika/sudoku/Board.hpp"
#include "antwika/sudoku/BoardLayout.hpp"
#include "antwika/sudoku/Widgets.hpp"

namespace antwika::sudoku::tests
{

    /**
     * @brief Find the pixel at the centre of a widget.
     *
     * Where a widget is, is the layout's business, so a test asks the
     * layout rather than deciding for itself -- which is exactly what
     * ui::Frame::rects is for, and one describe() answers it.
     *
     * @param frame The frame the widget was declared in.
     * @param id The widget to look for.
     * @return Its centre, or nothing when the frame declared no such
     * id.
     */
    [[nodiscard]] inline std::optional<gfx::Point> widgetCentre(
        const ui::Frame &frame, const ui::WidgetId id)
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
     * @brief Find the pixel at the centre of one square of the grid.
     *
     * Through the same layoutFor()/squareRect() pair the picture and
     * the hit-test both go through, so a test that clicks a square is
     * clicking where that square was actually drawn.
     *
     * @param frame The frame the board area was declared in.
     * @param square Which square.
     * @return Its centre, or nothing when no grid fits.
     */
    [[nodiscard]] inline std::optional<gfx::Point> squareCentre(
        const ui::Frame &frame, const Square square)
    {
        const auto area = frame.rects.find(widgets::kBoard);

        if (!area.has_value())
        {
            return std::nullopt;
        }

        const auto layout = layoutFor(*area);

        if (!layout.has_value())
        {
            return std::nullopt;
        }

        const auto rect = squareRect(*layout, square);

        return gfx::Point{
            .x = rect.origin.x
                 + static_cast<std::int32_t>(rect.size.width) / 2,
            .y = rect.origin.y
                 + static_cast<std::int32_t>(rect.size.height) / 2};
    }

} // namespace antwika::sudoku::tests
