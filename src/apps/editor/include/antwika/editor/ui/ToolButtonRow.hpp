#pragma once

#include <array>
#include <optional>

#include <antwika/enums/Enumeration.hpp>
#include <antwika/map/Settings.hpp>

#include "antwika/editor/ui/ToolPanel.hpp"

namespace antwika::editor
{

    struct ToolButtonRow final
    {
        ToolButton button;

        std::optional<map::Tool> tool;
    };

    inline constexpr std::array<ToolButtonRow, enums::kCount<ToolButton>>
        kToolButtonRows{{
            {ToolButton::Brush, map::Tool::Brush},
            {ToolButton::Picker, map::Tool::Picker},
            {ToolButton::FreeLook, std::nullopt},
            {ToolButton::Lighting, std::nullopt},
            {ToolButton::Lamp, map::Tool::Lamp},
            {ToolButton::RuleLines, std::nullopt},
            {ToolButton::Start, map::Tool::Start},
            {ToolButton::Exit, map::Tool::Exit},
            {ToolButton::Stamp, map::Tool::Stamp},
            {ToolButton::Figure, map::Tool::Figure},
            {ToolButton::PressurePlate, map::Tool::PressurePlate},
            {ToolButton::Key, map::Tool::Key},
            {ToolButton::Door, map::Tool::Door},
            {ToolButton::Checkpoint, map::Tool::Checkpoint},
            {ToolButton::Food, map::Tool::Food},
            {ToolButton::Water, map::Tool::Water},
            {ToolButton::Eraser, map::Tool::Eraser}}};

    static_assert(
        enums::tagsInOrder(kToolButtonRows, &ToolButtonRow::button));

}
