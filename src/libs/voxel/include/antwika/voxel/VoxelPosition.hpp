#pragma once

#include <compare>
#include <cstdint>

namespace antwika::voxel
{

    inline constexpr float kVoxelSide = 1.0F;

    struct VoxelPosition final
    {
        std::int32_t x = 0;
        std::int32_t y = 0;
        std::int32_t z = 0;

        [[nodiscard]] bool operator==(const VoxelPosition &other) const
            = default;

        [[nodiscard]] auto operator<=>(const VoxelPosition &other) const
            = default;
    };

}
