#include "antwika/worldgen/Expand.hpp"

#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/voxel/VoxelStairs.hpp>

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

        void lay(
            voxel::Voxels &laidVoxels,
            const voxel::VoxelPosition cornerPosition,
            const voxel::VoxelMaterial material,
            const voxel::VoxelPosition climbPosition)
        {
            for (const auto &[place, grownMaterial] :
                 voxel::cubeVoxels(
                     cornerPosition, material.kind, climbPosition))
            {
                laidVoxels[place] = voxel::VoxelMaterial{
                    .kind = grownMaterial.kind,
                    .facing = material.kind == voxel::Kind::Ramp
                                  ? material.facing
                                  : voxel::Facing::Any};
            }
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

            lay(
                laidVoxels,
                cornerOf(cubePosition),
                material,
                voxel::stepVectorFor(material.facing));
        }

        for (const auto &[cubePosition, material] : cubeVoxels)
        {
            if (!worksOutItsOwnWay(material))
            {
                continue;
            }

            const auto cornerPosition = cornerOf(cubePosition);

            lay(
                laidVoxels,
                cornerPosition,
                material,
                voxel::rampDirectionFor(laidVoxels, cornerPosition));
        }

        return laidVoxels;
    } // GCOVR_EXCL_LINE

}
