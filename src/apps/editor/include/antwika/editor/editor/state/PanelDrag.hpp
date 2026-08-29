#pragma once

#include <antwika/gfx/Size.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "antwika/editor/editor/state/PanelSizes.hpp"

namespace antwika::editor
{

    struct PanelDrag final
    {
        PanelSizes panelSizes{};

        widget::WidgetId heldEdgeWidget = widget::kNoWidget;

        gfx::Size windowSize{};
    };

}
