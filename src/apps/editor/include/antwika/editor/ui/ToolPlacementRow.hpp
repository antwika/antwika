#pragma once

#include <array>

#include <antwika/enums/Enumeration.hpp>
#include <antwika/map/Settings.hpp>

#include "antwika/editor/ui/ToolPlacement.hpp"

namespace antwika::editor
{

    struct ToolPlacementRow final
    {
        map::Tool tool;

        ToolPlacement placement;
    };

    inline constexpr std::array<
        ToolPlacementRow, enums::kCount<map::Tool>>
        kToolPlacementRows{{
            {map::Tool::Brush, ToolPlacement::Shape},
            {map::Tool::Picker, ToolPlacement::Shape},
            {map::Tool::Lamp, ToolPlacement::Lamp},
            {map::Tool::Start, ToolPlacement::StartOrExit},
            {map::Tool::Exit, ToolPlacement::StartOrExit},
            {map::Tool::Stamp, ToolPlacement::Stamp},
            {map::Tool::Figure, ToolPlacement::Figure},
            {map::Tool::PressurePlate, ToolPlacement::Plate},
            {map::Tool::Key, ToolPlacement::Gate},
            {map::Tool::Door, ToolPlacement::Gate},
            {map::Tool::Checkpoint, ToolPlacement::Gate},
            {map::Tool::Food, ToolPlacement::Gate},
            {map::Tool::Water, ToolPlacement::Gate},
            {map::Tool::Eraser, ToolPlacement::Shape}}};

    static_assert(
        enums::tagsInOrder(kToolPlacementRows, &ToolPlacementRow::tool));

    [[nodiscard]] constexpr ToolPlacement placementOf(
        const map::Tool tool) noexcept
    {
        return enums::lookup(kToolPlacementRows, tool).placement;
    }

}
