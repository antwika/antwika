#pragma once

#include <cstdint>

#include "antwika/ui/WidgetId.hpp"

namespace antwika::ui
{

    struct EdgeSpec final
    {
        WidgetId widgetId = kNoWidget;

        WidgetId panelWidget = kNoWidget;

        std::uint32_t minimum = 0;

        std::uint32_t maximum = 0;

        bool dragging = false;

        [[nodiscard]] bool operator==(const EdgeSpec &other) const =
            default;
    };

}
