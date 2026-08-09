#pragma once

#include <cstdint>
#include <optional>

#include <antwika/gfx/Point.hpp>
#include <antwika/ui/Frame.hpp>
#include <antwika/ui/support/WidgetCentre.hpp>

#include "antwika/sudoku/Board.hpp"
#include "antwika/sudoku/BoardLayout.hpp"
#include "antwika/sudoku/Widgets.hpp"

namespace antwika::sudoku::tests
{
    using antwika::ui::support::widgetCentre;

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
}
