#pragma once

#include <cstddef>

#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/WidgetId.hpp"

namespace antwika::ui
{

    struct ScrollSpec final
    {
        WidgetId widgetId = kNoWidget;

        Sizing widthSizing = kGrowSizing;

        Sizing heightSizing = kGrowSizing;

        std::size_t offset = 0;

        bool dragging = false;
    };

}
