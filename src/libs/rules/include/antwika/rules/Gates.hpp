#pragma once

#include <optional>
#include <span>
#include <vector>

#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/voxel/Voxels.hpp>

namespace antwika::rules
{

    [[nodiscard]] std::optional<voxel::VoxelPosition> getGateCubeContaining(
        std::span<const voxel::VoxelPosition> gatePositions,
        voxel::VoxelPosition position);

    [[nodiscard]] std::optional<voxel::VoxelPosition> getAdjacentDoor(
        std::span<const voxel::VoxelPosition> doorPositions,
        voxel::VoxelPosition standsInPosition);

    [[nodiscard]] std::vector<voxel::VoxelPosition> getDoorwayCells(
        std::span<const voxel::VoxelPosition> doorPositions,
        voxel::VoxelPosition cornerPosition);

    [[nodiscard]] bool isCubeOccupied(
        const voxel::Voxels &voxels, voxel::VoxelPosition cornerPosition);

}
