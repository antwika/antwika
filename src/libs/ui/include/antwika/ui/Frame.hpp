#pragma once

#include "antwika/ui/DrawList.hpp"
#include "antwika/ui/HoverTargets.hpp"
#include "antwika/ui/Interactions.hpp"
#include "antwika/ui/Overlays.hpp"
#include "antwika/ui/WidgetRects.hpp"

namespace antwika::ui
{

    struct Frame final
    {
        DrawList commands;

        Interactions interactions;

        WidgetRects rects;

        HoverTargets hoverTargets;

        Overlays overlays;
    };

}
