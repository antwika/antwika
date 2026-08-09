#pragma once

#include <antwika/ui/WidgetId.hpp>

namespace antwika::sudoku
{

    using antwika::ui::WidgetId;

    namespace widgets
    {
        inline constexpr WidgetId kSolve{1};

        inline constexpr WidgetId kBoard{2};
    }

    static_assert(
        antwika::ui::assertDistinct(widgets::kSolve, widgets::kBoard),
        "every widget in this application needs its own id");

}
