#pragma once

#include <array>
#include <optional>

#include <antwika/enums/Enumeration.hpp>
#include <antwika/voxel/VoxelMaterial.hpp>

#include "antwika/editor/Preferences.hpp"

#include "antwika/editor/ui/ToolGroup.hpp"
#include "antwika/editor/ui/ToolPanel.hpp"

namespace antwika::editor
{

    struct ToolButtonRow final
    {
        ToolButton button;

        ToolGroup group;

        Tool tool;

        std::optional<voxel::Kind> kind;
    };

    inline constexpr std::array<ToolButtonRow, enums::kCount<ToolButton>>
        kToolButtonRows{{
            {ToolButton::StoneCube,
             ToolGroup::Voxel,
             Tool::Brush,
             voxel::Kind::Normal},
            {ToolButton::WaterCube,
             ToolGroup::Voxel,
             Tool::Brush,
             voxel::Kind::Water},
            {ToolButton::RampCube,
             ToolGroup::Voxel,
             Tool::Brush,
             voxel::Kind::Ramp},
            {ToolButton::Picker,
             ToolGroup::Voxel,
             Tool::Picker,
             std::nullopt},
            {ToolButton::Stamp,
             ToolGroup::Voxel,
             Tool::Stamp,
             std::nullopt},
            {ToolButton::Rubber,
             ToolGroup::Voxel,
             Tool::Eraser,
             std::nullopt},
            {ToolButton::Select,
             ToolGroup::Entity,
             Tool::Select,
             std::nullopt},
            {ToolButton::Lamp,
             ToolGroup::Entity,
             Tool::Lamp,
             std::nullopt},
            {ToolButton::Start,
             ToolGroup::Entity,
             Tool::Start,
             std::nullopt},
            {ToolButton::Exit,
             ToolGroup::Entity,
             Tool::Exit,
             std::nullopt},
            {ToolButton::Character,
             ToolGroup::Entity,
             Tool::Character,
             std::nullopt},
            {ToolButton::Checkpoint,
             ToolGroup::Entity,
             Tool::Checkpoint,
             std::nullopt},
            {ToolButton::Food,
             ToolGroup::Entity,
             Tool::Food,
             std::nullopt},
            {ToolButton::Water,
             ToolGroup::Entity,
             Tool::Water,
             std::nullopt}}};

    static_assert(
        enums::tagsInOrder(kToolButtonRows, &ToolButtonRow::button));

    /**
     * @brief A button lights when its tool is the chosen one, and, for a
     * button that also names a cube kind, when that kind is the chosen one.
     */
    [[nodiscard]] constexpr bool isToolButtonActive(
        const ToolButton whichButton,
        const Tool chosenTool,
        const voxel::Kind chosenKind) noexcept
    {
        const auto &row = enums::lookup(kToolButtonRows, whichButton);

        return chosenTool == row.tool
               && (!row.kind.has_value() || *row.kind == chosenKind);
    }

}
