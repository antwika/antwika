#pragma once

#include <antwika/rng/IRng.hpp>
#include <antwika/voxel/VoxelCell.hpp>
#include <antwika/wfc/CompatibilityTable.hpp>

namespace antwika::worldgen
{

    struct VoxelBox final
    {
        voxel::VoxelCell lowCell{};

        voxel::VoxelCell highCell{};

        [[nodiscard]] bool operator==(const VoxelBox &other) const
            = default;
    };

}
