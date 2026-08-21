#include "antwika/rules/Gates.hpp"

#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/tilemap/TileEdges.hpp>

namespace antwika::rules
{

    std::optional<voxel::VoxelCell> gateCubeContaining(
        const std::span<const voxel::VoxelCell> gateCells,
        const voxel::VoxelCell cell)
    {
        const auto corner = voxel::cubeCornerOf(cell);

        for (const auto one : gateCells)
        {
            if (voxel::cubeCornerOf(one) == corner)
            {
                return corner;
            }
        }

        return std::nullopt;
    }

    std::optional<voxel::VoxelCell> adjacentDoor(
        const std::span<const voxel::VoxelCell> doorCells,
        const voxel::VoxelCell standsInCell)
    {
        const auto middle = voxel::cubeCornerOf(standsInCell);

        for (const auto way :
             {voxel::VoxelCell{.x = voxel::kCubeSide},
              voxel::VoxelCell{.x = -voxel::kCubeSide},
              voxel::VoxelCell{.z = voxel::kCubeSide},
              voxel::VoxelCell{.z = -voxel::kCubeSide}})
        {
            const auto foundCube = gateCubeContaining(
                doorCells,
                voxel::VoxelCell{
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

    std::vector<voxel::VoxelCell> doorwayCells(
        const std::span<const voxel::VoxelCell> doorCells,
        const voxel::VoxelCell cornerCell)
    {
        std::vector<voxel::VoxelCell> cells;

        for (const auto door : doorCells)
        {
            if (voxel::cubeCornerOf(door).x == cornerCell.x
                && voxel::cubeCornerOf(door).z == cornerCell.z)
            {
                cells.push_back(door);
            }
        }

        return cells;
    } // GCOVR_EXCL_LINE

    bool cubeOccupied(
        const std::span<const voxel::VoxelCell> voxels,
        const voxel::VoxelCell cornerCell)
    {
        for (const auto voxel : voxels)
        {
            if (voxel::cubeCornerOf(voxel) == cornerCell)
            {
                return true;
            }
        }

        return false;
    }

}
