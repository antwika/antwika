#pragma once

#include <array>
#include <cstddef>

#include <antwika/enums/Enumeration.hpp>

#include "antwika/editor/ui/ToolButtonRow.hpp"
#include "antwika/editor/ui/ToolGroup.hpp"
#include "antwika/editor/ui/ToolPanel.hpp"

namespace antwika::editor
{

    struct ToolGroupMembers final
    {
        std::array<ToolButton, enums::kCount<ToolButton>> buttons{};

        std::size_t count = 0;
    };

    /**
     * @brief The buttons of one group, in the order the rows name them.
     */
    [[nodiscard]] constexpr ToolGroupMembers getToolsIn(
        const ToolGroup whichGroup) noexcept
    {
        ToolGroupMembers members;

        for (const auto &row : kToolButtonRows)
        {
            if (row.group == whichGroup)
            {
                members.buttons[members.count] = row.button;
                ++members.count;
            }
        }

        return members;
    }

}
