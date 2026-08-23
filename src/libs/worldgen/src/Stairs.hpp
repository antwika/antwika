#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include <antwika/rng/IRng.hpp>
#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/worldgen/ruleset/CompiledRuleset.hpp>

#include "Lattice.hpp"
#include "antwika/worldgen/ChunkShape.hpp"
#include "antwika/worldgen/Ruleset.hpp"

namespace antwika::worldgen::detail
{

    [[nodiscard]] std::size_t getWalkSteps(ChunkShape shape);

    inline constexpr std::uint32_t kRouteAttempts = 8;

    struct StairResult final
    {
        bool climbed = false;

        std::vector<std::size_t> landings{};

        voxel::VoxelPosition stuckPosition{};
    };

    [[nodiscard]] bool fits(
        const Board &board,
        std::size_t cell,
        std::span<const std::size_t> wantedCells);

    [[nodiscard]] std::int32_t getHighestTerrace(
        const CompiledRuleset &compiledRuleset, ChunkShape shape);

    [[nodiscard]] StairResult layWays(
        const CompiledRuleset &compiledRuleset,
        ChunkShape shape,
        Board &board,
        std::uint32_t wantedCount,
        rng::IRng &rng);

}
