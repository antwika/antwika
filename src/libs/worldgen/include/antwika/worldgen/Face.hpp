#pragma once

#include <cstdint>
#include <antwika/rng/IRng.hpp>
#include <antwika/voxel/VoxelCell.hpp>
#include <antwika/wfc/CompatibilityTable.hpp>
#include "antwika/worldgen/ChunkShape.hpp"

namespace antwika::worldgen
{

    enum class Face : std::uint8_t
    {
        East,
        West,
        Up,
        Down,
        North,
        South,
    };

}
