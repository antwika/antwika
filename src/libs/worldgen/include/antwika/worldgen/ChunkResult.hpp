#pragma once

#include <vector>
#include <antwika/rng/IRng.hpp>
#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/voxel/Voxels.hpp>
#include <antwika/worldgen/ruleset/CompiledRuleset.hpp>
#include "antwika/worldgen/ChunkOutcome.hpp"
#include "antwika/worldgen/ChunkShape.hpp"
#include "antwika/worldgen/Ruleset.hpp"

namespace antwika::worldgen
{

    struct ChunkResult final
    {
        ChunkOutcome outcome = ChunkOutcome::Grown;

        voxel::Voxels cubeVoxels{};

        std::vector<voxel::VoxelPosition> culpritPositions{};

        [[nodiscard]] bool operator==(const ChunkResult &other) const
            = default;
    };

}
