#include "antwika/rules/MarkerCubes.hpp"

#include <antwika/voxel/VoxelCube.hpp>

namespace antwika::rules
{

    std::optional<voxel::VoxelPosition> getMarkerCubeContaining(
        const std::span<const voxel::VoxelPosition> markerPositions,
        const voxel::VoxelPosition position)
    {
        const auto corner = voxel::cubeCornerOf(position);

        for (const auto one : markerPositions)
        {
            if (voxel::cubeCornerOf(one) == corner)
            {
                return corner;
            }
        }

        return std::nullopt;
    }

}
