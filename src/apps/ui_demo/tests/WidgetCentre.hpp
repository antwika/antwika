#pragma once

#include <cstdint>

#include <antwika/ui/WidgetId.hpp>
#include <antwika/ui/support/WidgetCentre.hpp>

namespace antwika::ui_demo::tests
{
    using antwika::ui::support::widgetCentre;

    [[nodiscard]] inline ui::WidgetId optionWidget(
        ui::WidgetId base, std::uint64_t index)
    {
        return static_cast<ui::WidgetId>(
            static_cast<std::uint64_t>(base) + index);
    }
}
