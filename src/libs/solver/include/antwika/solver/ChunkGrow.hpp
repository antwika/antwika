#pragma once

#include <string>
#include <vector>

#include <antwika/worldgen/ChunkShape.hpp>
#include <antwika/worldgen/Grow.hpp>

#include <antwika/voxel/VoxelCell.hpp>

namespace antwika::solver
{

    [[nodiscard]] std::vector<voxel::VoxelCell> hintsFrom(
        const std::vector<voxel::VoxelCell> &voxels,
        worldgen::ChunkShape shape,
        voxel::VoxelCell originPointCell);

    [[nodiscard]] std::vector<voxel::VoxelCell> withChunkSpliced(
        std::vector<voxel::VoxelCell> pileCell,
        worldgen::VoxelBox box,
        const std::vector<voxel::VoxelCell> &grownCell);

    [[nodiscard]] std::string growTrouble(
        const worldgen::ChunkResult &result);

}
