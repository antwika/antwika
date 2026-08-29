#pragma once

#include <cstdint>

#include <antwika/rng/IRng.hpp>
#include <antwika/voxel/VoxelCell.hpp>
#include <antwika/wfc/CompatibilityTable.hpp>

#include "antwika/worldgen/ChunkShape.hpp"

namespace antwika::worldgen
{

    enum class Role : std::uint8_t
    {
        Room,
        Perch,
        Bear,
        Step,
        Land,
    };

}
