#pragma once

#include <array>

#include <antwika/enums/Enumeration.hpp>

#include "antwika/voxel/VoxelMaterial.hpp"
#include "antwika/voxel/VoxelPosition.hpp"

namespace antwika::voxel
{

    struct FacingTraits final
    {
        Facing facing;

        VoxelPosition stepPosition;
    };

    inline constexpr std::array<FacingTraits, enums::kCount<Facing>>
        kFacingTraits{{
            {.facing = Facing::Any, .stepPosition = VoxelPosition{}},
            {.facing = Facing::East, .stepPosition = VoxelPosition{.x = 1}},
            {.facing = Facing::West, .stepPosition = VoxelPosition{.x = -1}},
            {.facing = Facing::North, .stepPosition = VoxelPosition{.z = -1}},
            {.facing = Facing::South, .stepPosition = VoxelPosition{.z = 1}}}};

    static_assert(enums::tagsInOrder(kFacingTraits, &FacingTraits::facing));

    [[nodiscard]] constexpr VoxelPosition stepOf(const Facing facing) noexcept
    {
        return enums::lookup(kFacingTraits, facing).stepPosition;
    }

    inline constexpr std::array<Facing, enums::kCount<Facing> - 1>
        kCardinalFacings{
            Facing::East, Facing::West, Facing::North, Facing::South};

}
