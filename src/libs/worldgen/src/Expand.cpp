#include "antwika/worldgen/Expand.hpp"

#include <antwika/voxel/VoxelCube.hpp>

namespace antwika::worldgen
{

    namespace
    {

        [[nodiscard]] bool worksOutItsOwnWay(
            const voxel::VoxelMaterial material)
        {
            return material.kind == voxel::Kind::Ramp
                   && material.facing == voxel::Facing::Any;
        }

        [[nodiscard]] voxel::VoxelPosition cornerOf(
            const voxel::VoxelPosition cubePosition)
        {
            return voxel::VoxelPosition{
                .x = cubePosition.x * voxel::kCubeSide,
                .y = cubePosition.y * voxel::kCubeSide,
                .z = cubePosition.z * voxel::kCubeSide};
        }

    }

    voxel::Voxels chunkVoxels(const voxel::Voxels &cubeVoxels)
    {
        voxel::Voxels laidVoxels;

        for (const auto &[cubePosition, material] : cubeVoxels)
        {
            if (worksOutItsOwnWay(material))
            {
                continue;
            }

            laidVoxels = voxel::withBlockAt(
                laidVoxels,
                cornerOf(cubePosition),
                material.kind,
                material.facing);
        }

        for (const auto &[cubePosition, material] : cubeVoxels)
        {
            if (!worksOutItsOwnWay(material))
            {
                continue;
            }

            laidVoxels = voxel::withBlockAt(
                laidVoxels,
                cornerOf(cubePosition),
                material.kind,
                material.facing);
        }

        return laidVoxels;
    } // GCOVR_EXCL_LINE

}
