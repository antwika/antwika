#pragma once

#include <optional>
#include <span>
#include <vector>

#include <antwika/voxel/VoxelCell.hpp>

namespace antwika::rules
{

    [[nodiscard]] std::optional<voxel::VoxelCell> gateCubeContaining(
        std::span<const voxel::VoxelCell> gateCells,
        voxel::VoxelCell cell);

    [[nodiscard]] std::optional<voxel::VoxelCell> adjacentDoor(
        std::span<const voxel::VoxelCell> doorCells,
        voxel::VoxelCell standsInCell);

    [[nodiscard]] std::vector<voxel::VoxelCell> doorwayCells(
        std::span<const voxel::VoxelCell> doorCells,
        voxel::VoxelCell cornerCell);

    [[nodiscard]] bool cubeOccupied(
        std::span<const voxel::VoxelCell> voxels,
        voxel::VoxelCell cornerCell);

}
