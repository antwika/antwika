#pragma once

#include <cstdint>

#include <antwika/rng/IRng.hpp>
#include <antwika/voxel/VoxelCell.hpp>
#include <antwika/worldgen/ruleset/CompiledRuleset.hpp>

#include "antwika/worldgen/ChunkShape.hpp"
#include "antwika/worldgen/Ruleset.hpp"

namespace antwika::worldgen
{

    enum class ChunkOutcome : std::uint8_t
    {
        Grown,

        HintOutside,

        HintUnknown,

        HintsConflict,

        NoWayUp,

        Unsatisfiable,

        LimitExceeded,
    };

}
