#pragma once

#include <cstdint>
#include <antwika/rng/IRng.hpp>
#include <antwika/voxel/VoxelCell.hpp>
#include <antwika/wfc/CompatibilityTable.hpp>
#include "antwika/worldgen/ChunkShape.hpp"

namespace antwika::worldgen
{

    enum class Socket : std::uint8_t
    {
        OpenSide,
        RoomSide,
        Facade,
        LandingSide,
        Buried,
        WaterSide,
        StairSideEast,
        StairSideWest,
        StairSideNorth,
        StairSideSouth,
        NeedsRoot,
        NeedsLanding,
        NeedsApproach,
        NeedsOpen,
        NeedsBack,
        Sky,
        Carries,
        Terrace,
        StairHead,
        Rung,
        WaterTop,
        Floats,
        Stands,
        Rests,
        Hangs,
        Rooted,
        Climbs,
        Grips,
        Submerged,
    };

}
