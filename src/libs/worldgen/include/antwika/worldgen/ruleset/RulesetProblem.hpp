#pragma once

#include <cstddef>

#include <antwika/rng/IRng.hpp>
#include <antwika/voxel/VoxelCell.hpp>
#include <antwika/wfc/CompatibilityTable.hpp>

#include "antwika/worldgen/ChunkShape.hpp"
#include "antwika/worldgen/Face.hpp"
#include "antwika/worldgen/RulesetFault.hpp"

namespace antwika::worldgen
{

    struct RulesetProblem final
    {
        RulesetFault fault = RulesetFault::NoPrototypes;

        std::size_t subjectIndex = 0;

        Face face = Face::East;

        [[nodiscard]] bool operator==(const RulesetProblem &other) const
            = default;
    };

}
