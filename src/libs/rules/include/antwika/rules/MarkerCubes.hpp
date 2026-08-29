#pragma once

#include <optional>
#include <span>

#include <antwika/voxel/VoxelPosition.hpp>

namespace antwika::rules
{

    [[nodiscard]] std::optional<voxel::VoxelPosition> getMarkerCubeContaining(
        std::span<const voxel::VoxelPosition> markerPositions,
        voxel::VoxelPosition position);

}
