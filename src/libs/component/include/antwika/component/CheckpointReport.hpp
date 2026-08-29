#pragma once

#include <antwika/voxel/VoxelPosition.hpp>

namespace antwika::component
{

    struct CheckpointReport final
    {
        voxel::VoxelPosition position{};
    };

}
