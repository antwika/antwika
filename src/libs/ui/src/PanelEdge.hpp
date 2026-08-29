#pragma once

#include <cstddef>
#include <cstdint>

#include "antwika/ui/Axis.hpp"
#include "antwika/ui/WidgetId.hpp"

namespace antwika::ui::detail
{

    struct PanelEdge final
    {
        WidgetId widgetId = kNoWidget;

        WidgetId panelWidget = kNoWidget;

        std::size_t bar = 0;

        Axis axis = Axis::Row;

        std::uint32_t minimum = 0;

        std::uint32_t maximum = 0;

        bool dragging = false;
    };

}
