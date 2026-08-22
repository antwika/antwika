#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <antwika/enums/Enumeration.hpp>
#include <antwika/voxel/VoxelPosition.hpp>

namespace antwika::component
{

    enum class ItemKind : std::uint8_t
    {
        Food,
        Water,
    };

    [[nodiscard]] constexpr ItemKind lastEnumerator(ItemKind) noexcept
    {
        return ItemKind::Water;
    }

    inline constexpr std::size_t kItemKindCount =
        enums::kCount<ItemKind>;

    inline constexpr std::array<ItemKind, kItemKindCount>
        kEveryItemKind = enums::kAll<ItemKind>;

    struct Item final
    {
        voxel::VoxelPosition position{};

        std::uint8_t kind = 0;
    };

}
