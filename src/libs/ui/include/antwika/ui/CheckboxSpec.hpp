#pragma once

#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/WidgetId.hpp"

namespace antwika::ui
{

    struct CheckboxSpec final
    {
        WidgetId widgetId = kNoWidget;

        Sizing widthSizing = kFitSizing;

        bool checked = false;
    };

}
