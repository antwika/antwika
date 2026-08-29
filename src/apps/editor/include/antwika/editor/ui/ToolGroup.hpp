#pragma once

#include <array>
#include <cstdint>
#include <string_view>

#include <antwika/enums/Enumeration.hpp>

namespace antwika::editor
{

    enum class ToolGroup : std::uint8_t
    {
        Voxel,
        Entity,
    };

    [[nodiscard]] constexpr ToolGroup getLastEnumerator(ToolGroup) noexcept
    {
        return ToolGroup::Entity;
    }

    inline constexpr std::array<ToolGroup, enums::kCount<ToolGroup>>
        kEveryToolGroup = enums::kAll<ToolGroup>;

    struct ToolGroupRow final
    {
        ToolGroup group;

        std::string_view title;
    };

    inline constexpr std::array<ToolGroupRow, enums::kCount<ToolGroup>>
        kToolGroupRows{{
            {ToolGroup::Voxel, "Voxel tools"},
            {ToolGroup::Entity, "Entity tools"}}};

    static_assert(
        enums::tagsInOrder(kToolGroupRows, &ToolGroupRow::group));

    [[nodiscard]] constexpr std::string_view getToolGroupTitle(
        const ToolGroup whichGroup) noexcept
    {
        return enums::lookup(kToolGroupRows, whichGroup).title;
    }

}
