#pragma once

#include <antwika/enums/Enumeration.hpp>
#include <antwika/map/Settings.hpp>

#include "antwika/editor/ui/ToolButtonRow.hpp"
#include "antwika/editor/ui/ToolPanel.hpp"

namespace antwika::editor
{

    struct ToolToggles final
    {
        bool freeLook = false;

        bool lighting = false;

        bool showRuleLines = false;
    };

    [[nodiscard]] constexpr bool isToolButtonActive(
        const ToolButton whichButton,
        const map::Tool chosenTool,
        const ToolToggles toggles) noexcept
    {
        const auto shownTool =
            enums::lookup(kToolButtonRows, whichButton).tool;

        if (shownTool.has_value())
        {
            return chosenTool == *shownTool;
        }

        if (whichButton == ToolButton::Lighting)
        {
            return toggles.lighting;
        }

        if (whichButton == ToolButton::RuleLines)
        {
            return toggles.showRuleLines;
        }

        return toggles.freeLook;
    }

}
