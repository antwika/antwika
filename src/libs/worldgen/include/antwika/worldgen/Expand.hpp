#pragma once

#include <vector>

#include <antwika/voxel/VoxelCell.hpp>

namespace antwika::worldgen
{

    [[nodiscard]] std::vector<voxel::VoxelCell> chunkVoxels(
        const std::vector<voxel::VoxelCell> &cubeCells);

}
