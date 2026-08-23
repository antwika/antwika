#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include <antwika/rng/IRng.hpp>
#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/worldgen/ruleset/CompiledRuleset.hpp>

#include "antwika/worldgen/ChunkOutcome.hpp"
#include "antwika/worldgen/ChunkRequest.hpp"
#include "antwika/worldgen/ChunkResult.hpp"
#include "antwika/worldgen/ChunkShape.hpp"
#include "antwika/worldgen/Ruleset.hpp"

namespace antwika::worldgen
{

    inline constexpr std::size_t kMaxReportedCulprits = 8;

    inline constexpr std::uint64_t kFillSalt = 0x9E3779B97F4A7C15ULL;

    [[nodiscard]] ChunkResult getGrowChunk(
        const CompiledRuleset &compiledRuleset, const ChunkRequest &request);

    [[nodiscard]] ChunkResult getGrowChunk(
        const CompiledRuleset &compiledRuleset,
        const ChunkRequest &request,
        rng::IRng &waysRng,
        rng::IRng &fillRng);

}
