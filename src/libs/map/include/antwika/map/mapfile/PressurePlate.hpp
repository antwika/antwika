#pragma once

#include <vector>

#include <antwika/voxel/VoxelPosition.hpp>

namespace antwika::map
{

    struct PressurePlate final
    {
        voxel::VoxelPosition position{};

        std::vector<voxel::VoxelPosition> togglePositions{};

        [[nodiscard]] bool operator==(const PressurePlate &other) const
            = default;
    };

}
