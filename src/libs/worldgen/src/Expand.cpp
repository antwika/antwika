#include "antwika/worldgen/Expand.hpp"

#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/voxel/VoxelStairs.hpp>

#include "antwika/worldgen/WorldgenError.hpp"

namespace antwika::worldgen
{

    std::vector<voxel::VoxelCell> chunkVoxels(
        const std::vector<voxel::VoxelCell> &cubeCells)
    {
        std::vector<voxel::VoxelCell> laidCells;
        laidCells.reserve(cubeCells.size() * voxel::kCubeVoxels);

        for (const voxel::VoxelCell cube : cubeCells)
        {
            if (cube.kind == voxel::Kind::Ramp
                && cube.facing == voxel::Facing::Any)
            {
                throw WorldgenError(
                    "chunkVoxels: a ramp must say which way it climbs");
            }

            const voxel::VoxelCell cornerCell{
                .x = cube.x * voxel::kCubeSide,
                .y = cube.y * voxel::kCubeSide,
                .z = cube.z * voxel::kCubeSide};

            for (voxel::VoxelCell laidVoxel : voxel::cubeVoxels(
                     cornerCell, cube.kind, voxel::stepVectorFor(cube.facing)))
            {
                laidVoxel.facing = cube.kind == voxel::Kind::Ramp
                                       ? cube.facing
                                       : voxel::Facing::Any;

                laidCells.push_back(laidVoxel);
            }
        }

        return laidCells;
    } // GCOVR_EXCL_LINE

}
