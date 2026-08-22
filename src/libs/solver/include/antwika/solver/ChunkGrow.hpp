#pragma once

#include <string>
#include <vector>

#include <antwika/worldgen/ChunkShape.hpp>
#include <antwika/worldgen/Grow.hpp>

#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/voxel/Voxels.hpp>

namespace antwika::solver
{

    [[nodiscard]] voxel::Voxels hintsFrom(
        const voxel::Voxels &voxels,
        worldgen::ChunkShape shape,
        voxel::VoxelPosition originPointPosition);

    [[nodiscard]] voxel::Voxels withChunkSpliced(
        voxel::Voxels pileVoxels,
        worldgen::VoxelBox box,
        const voxel::Voxels &grownVoxels);

    [[nodiscard]] std::string growTrouble(
        const worldgen::ChunkResult &result);

}
