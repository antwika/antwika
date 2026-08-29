#pragma once

#include <cstdint>
#include <vector>

#include <antwika/rng/IRng.hpp>
#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/voxel/Voxels.hpp>
#include <antwika/worldgen/ruleset/CompiledRuleset.hpp>

#include "antwika/worldgen/ChunkShape.hpp"
#include "antwika/worldgen/Ruleset.hpp"

namespace antwika::worldgen
{

    inline constexpr std::uint64_t kMaxSteps = 250'000;

    inline constexpr std::uint32_t kDefaultWays = 5;

    struct ChunkRequest final
    {
        std::uint64_t seed = 0;

        ChunkShape shape{};

        voxel::VoxelPosition originPosition{};

        voxel::Voxels hintVoxels{};

        std::uint32_t ways = kDefaultWays;

        std::uint64_t maxSteps = kMaxSteps;
    };

}
