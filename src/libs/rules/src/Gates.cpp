#include "antwika/rules/Gates.hpp"

#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/tilemap/TileEdges.hpp>

namespace antwika::rules
{

    std::optional<voxel::VoxelPosition> getGateCubeContaining(
        const std::span<const voxel::VoxelPosition> gatePositions,
        const voxel::VoxelPosition position)
    {
        const auto corner = voxel::cubeCornerOf(position);

        for (const auto one : gatePositions)
        {
            if (voxel::cubeCornerOf(one) == corner)
            {
                return corner;
            }
        }

        return std::nullopt;
    }

    std::optional<voxel::VoxelPosition> getAdjacentDoor(
        const std::span<const voxel::VoxelPosition> doorPositions,
        const voxel::VoxelPosition standsInPosition)
    {
        const auto middle = voxel::cubeCornerOf(standsInPosition);

        for (const auto way :
             {voxel::VoxelPosition{.x = voxel::kCubeSide},
              voxel::VoxelPosition{.x = -voxel::kCubeSide},
              voxel::VoxelPosition{.z = voxel::kCubeSide},
              voxel::VoxelPosition{.z = -voxel::kCubeSide}})
        {
            const auto foundCube = getGateCubeContaining(
                doorPositions,
                voxel::VoxelPosition{
                    .x = middle.x + way.x,
                    .y = middle.y,
                    .z = middle.z + way.z});

            if (foundCube.has_value())
            {
                return foundCube;
            }
        }

        return std::nullopt;
    }

    std::vector<voxel::VoxelPosition> getDoorwayCells(
        const std::span<const voxel::VoxelPosition> doorPositions,
        const voxel::VoxelPosition cornerPosition)
    {
        std::vector<voxel::VoxelPosition> positions;

        for (const auto door : doorPositions)
        {
            if (voxel::cubeCornerOf(door).x == cornerPosition.x
                && voxel::cubeCornerOf(door).z == cornerPosition.z)
            {
                positions.push_back(door);
            }
        }

        return positions;
    } // GCOVR_EXCL_LINE

    bool isCubeOccupied(
        const voxel::Voxels &voxels,
        const voxel::VoxelPosition cornerPosition)
    {
        for (const auto &[position, material] : voxels)
        {
            if (voxel::cubeCornerOf(position) == cornerPosition)
            {
                return true;
            }
        }

        return false;
    }

}
