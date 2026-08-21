#pragma once

#include <cstdint>

#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/WidgetId.hpp"

namespace antwika::ui
{

    struct SliderSpec final
    {
        WidgetId widgetId = kNoWidget;

        Sizing widthSizing = kGrowSizing;

        std::uint32_t value = 0;

        std::uint32_t range = 100;

        bool dragging = false;

        [[nodiscard]] bool operator==(const SliderSpec &other) const =
            default;
    };

}
