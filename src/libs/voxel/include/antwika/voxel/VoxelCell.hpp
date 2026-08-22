#pragma once

#include <compare>
#include <cstdint>

#include "antwika/voxel/VoxelMaterial.hpp"
#include "antwika/voxel/VoxelPosition.hpp"

namespace antwika::voxel
{

    struct VoxelCell final
    {
        std::int32_t x = 0;
        std::int32_t y = 0;
        std::int32_t z = 0;

        Kind kind = Kind::Normal;

        Facing facing = Facing::Any;

        [[nodiscard]] VoxelPosition position() const
        {
            return VoxelPosition{.x = x, .y = y, .z = z};
        }

        [[nodiscard]] VoxelMaterial material() const
        {
            return VoxelMaterial{.kind = kind, .facing = facing};
        }

        [[nodiscard]] bool operator==(const VoxelCell &other) const
        {
            return x == other.x && y == other.y && z == other.z;
        }

        [[nodiscard]] std::strong_ordering operator<=>(
            const VoxelCell &other) const
        {
            if (const auto xOrder = x <=> other.x; xOrder != 0)
            {
                return xOrder;
            }

            if (const auto yOrder = y <=> other.y; yOrder != 0)
            {
                return yOrder;
            }

            return z <=> other.z;
        }
    };

    [[nodiscard]] inline VoxelCell voxelCellAt(
        const VoxelPosition position, const VoxelMaterial material)
    {
        return VoxelCell{
            .x = position.x,
            .y = position.y,
            .z = position.z,
            .kind = material.kind,
            .facing = material.facing};
    }

}
