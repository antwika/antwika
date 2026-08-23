#pragma once

#include <antwika/tilemap/Tilemap.hpp>
#include <antwika/voxel/VoxelCube.hpp>

namespace antwika::map::mapfile
{

    struct CornerRow final
    {
        tilemap::Tile tile{};

        voxel::Corner corner{};

        bool filled = false;
    };

}
