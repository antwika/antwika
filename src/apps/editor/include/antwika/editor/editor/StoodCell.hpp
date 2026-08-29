#pragma once

#include <cmath>
#include <cstdint>

#include <antwika/gfx/Math3D.hpp>
#include <antwika/voxel/VoxelPosition.hpp>

namespace antwika::editor
{

    /**
     * @brief The cell a thing standing at this position occupies.
     */
    [[nodiscard]] inline voxel::VoxelPosition getStoodCell(
        const gfx::Vec3 stoodPosition)
    {
        return voxel::VoxelPosition{
            .x = static_cast<std::int32_t>(
                std::floor(stoodPosition.x / voxel::kVoxelSide)),
            .y = static_cast<std::int32_t>(
                std::floor(stoodPosition.y / voxel::kVoxelSide)),
            .z = static_cast<std::int32_t>(
                std::floor(stoodPosition.z / voxel::kVoxelSide))};
    }

}
