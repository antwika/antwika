#pragma once

#include <vector>
#include <antwika/rng/IRng.hpp>
#include <antwika/voxel/VoxelCell.hpp>
#include <antwika/worldgen/ruleset/CompiledRuleset.hpp>
#include "antwika/worldgen/ChunkOutcome.hpp"
#include "antwika/worldgen/ChunkShape.hpp"
#include "antwika/worldgen/Ruleset.hpp"

namespace antwika::worldgen
{

    struct ChunkResult final
    {
        ChunkOutcome outcome = ChunkOutcome::Grown;

        std::vector<voxel::VoxelCell> cubeCells{};

        std::vector<voxel::VoxelCell> culpritCells{};

        [[nodiscard]] bool operator==(const ChunkResult &other) const
            = default;
    };

}
