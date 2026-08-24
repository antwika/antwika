#pragma once

#include <cstddef>
#include <cstdint>

#include "antwika/voxel/VoxelPosition.hpp"

namespace antwika::voxel
{

    struct VoxelPositionHash final
    {
        [[nodiscard]] std::size_t operator()(
            const VoxelPosition position) const noexcept
        {
            const auto spread = [](const std::int32_t part,
                                   const std::uint64_t odd)
            {
                return static_cast<std::uint64_t>(
                           static_cast<std::uint32_t>(part))
                       * odd;
            };
            const auto hashValue = spread(position.x, 0x9E3779B97F4A7C15ULL)
                                   ^ spread(position.y, 0xC2B2AE3D27D4EB4FULL)
                                   ^ spread(position.z, 0x165667B19E3779F9ULL);

            return static_cast<std::size_t>(hashValue ^ (hashValue >> 29));
        }
    };

}
