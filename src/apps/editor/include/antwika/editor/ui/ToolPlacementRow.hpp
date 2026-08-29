#pragma once

#include <array>

#include <antwika/enums/Enumeration.hpp>

#include "antwika/editor/Preferences.hpp"

#include "antwika/editor/ui/ToolPlacement.hpp"

namespace antwika::editor
{

    struct ToolPlacementRow final
    {
        Tool tool;

        ToolPlacement placement;
    };

    inline constexpr std::array<
        ToolPlacementRow, enums::kCount<Tool>>
        kToolPlacementRows{{
            {Tool::Select, ToolPlacement::Select},
            {Tool::Brush, ToolPlacement::Shape},
            {Tool::Picker, ToolPlacement::Shape},
            {Tool::Lamp, ToolPlacement::Lamp},
            {Tool::Start, ToolPlacement::StartOrExit},
            {Tool::Exit, ToolPlacement::StartOrExit},
            {Tool::Stamp, ToolPlacement::Stamp},
            {Tool::Character, ToolPlacement::Character},
            {Tool::Checkpoint, ToolPlacement::Marker},
            {Tool::Food, ToolPlacement::Marker},
            {Tool::Water, ToolPlacement::Marker},
            {Tool::Eraser, ToolPlacement::Shape}}};

    static_assert(
        enums::tagsInOrder(kToolPlacementRows, &ToolPlacementRow::tool));

    [[nodiscard]] constexpr ToolPlacement placementOf(
        const Tool tool) noexcept
    {
        return enums::lookup(kToolPlacementRows, tool).placement;
    }

}
