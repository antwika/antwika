#pragma once

#include <antwika/rng/IRng.hpp>
#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/wfc/CompatibilityTable.hpp>

namespace antwika::worldgen
{

    struct VoxelBox final
    {
        voxel::VoxelPosition lowPosition{};

        voxel::VoxelPosition highPosition{};

        [[nodiscard]] bool operator==(const VoxelBox &other) const
            = default;
    };

}
