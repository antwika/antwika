#pragma once

#include <cstdint>
#include <vector>

#include <antwika/voxel/VoxelPosition.hpp>

namespace antwika::editor
{

    struct GrowSetup final
    {
        std::uint64_t seed = 0;

        std::vector<voxel::VoxelPosition> troublePositions;
    };

}
