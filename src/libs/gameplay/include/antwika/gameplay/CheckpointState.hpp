#pragma once

#include <optional>

#include <antwika/map/MapFile.hpp>
#include <antwika/voxel/VoxelPosition.hpp>

namespace antwika::gameplay
{

    struct CheckpointState final
    {
        std::optional<map::Placement> placement;

        std::optional<voxel::VoxelPosition> onPosition;
    };

}
