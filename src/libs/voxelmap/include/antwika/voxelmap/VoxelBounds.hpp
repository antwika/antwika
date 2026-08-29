#pragma once

#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/voxel/Voxels.hpp>

namespace antwika::voxelmap
{

    struct VoxelBounds final
    {
        voxel::VoxelPosition lowestPosition{};

        voxel::VoxelPosition highestPosition{};

        [[nodiscard]] bool operator==(const VoxelBounds &other) const
            = default;
    };

    [[nodiscard]] VoxelBounds boundsOf(const voxel::Voxels &voxels);

}
