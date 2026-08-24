#pragma once

#include <string_view>
#include <vector>

#include <antwika/voxel/VoxelPosition.hpp>

#include "MapFileShared.hpp"

namespace antwika::map::mapfile
{

    struct GateRow final
    {
        std::string_view key;

        Marker marker;
    };

}
