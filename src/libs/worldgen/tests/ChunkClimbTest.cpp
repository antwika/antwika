#include <gtest/gtest.h>

#include <vector>

#include <antwika/pathfinding/GridPos.hpp>
#include <antwika/pathfinding/Path.hpp>
#include <antwika/voxel/VoxelCell.hpp>
#include <antwika/worldgen/ChunkShape.hpp>
#include <antwika/worldgen/CityRuleset.hpp>
#include <antwika/worldgen/Grow.hpp>
#include <antwika/worldgen/Ruleset.hpp>
#include <antwika/worldgen/fakes/FakeChunkWalkGraph.hpp>
#include <antwika/worldgen/ruleset/CompiledRuleset.hpp>

#include "Stairs.hpp"

using antwika::pathfinding::GridPos;
using antwika::pathfinding::pathBetween;
using antwika::voxel::Kind;
using antwika::voxel::VoxelCell;
using antwika::voxel::voxelsOf;
using antwika::voxel::VoxelPosition;
using antwika::worldgen::ChunkOutcome;
using antwika::worldgen::ChunkRequest;
using antwika::worldgen::ChunkResult;
using antwika::worldgen::ChunkShape;
using antwika::worldgen::cityRuleset;
using antwika::worldgen::CompiledRuleset;
using antwika::worldgen::cubeCount;
using antwika::worldgen::growChunk;
using antwika::worldgen::fakes::FakeChunkWalkGraph;

namespace
{
    constexpr ChunkShape kSmallShape{.width = 6, .depth = 6, .height = 16};

    const CompiledRuleset &city()
    {
        static const CompiledRuleset compiledRuleset(cityRuleset());

        return compiledRuleset;
    }

    [[nodiscard]] std::int32_t roofOf()
    {
        return antwika::worldgen::detail::highestTerrace(city(), kSmallShape);
    }

    [[nodiscard]] bool climbable(
        const ChunkResult &result, const std::int32_t roof)
    {
        const FakeChunkWalkGraph graph(kSmallShape, result.cubeVoxels);
        const auto cap = static_cast<std::uint64_t>(cubeCount(kSmallShape));

        for (const GridPos street : graph.streets())
        {
            for (std::int32_t x = 0; x < kSmallShape.width; ++x)
            {
                for (std::int32_t z = 0; z < kSmallShape.depth; ++z)
                {
                    const GridPos topPos{.x = x, .y = roof, .z = z};

                    if (graph.roomy(topPos)
                        && pathBetween(graph, street, topPos, cap).has_value())
                    {
                        return true;
                    }
                }
            }
        }

        return false;
    }
}

TEST(ChunkClimbTest, Grow_LeavesAWayFromTheStreetToTheHighestTerrace)
{
    for (std::uint64_t seed = 1; seed <= 4; ++seed)
    {
        const auto result =
            growChunk(city(), ChunkRequest{.seed = seed, .shape = kSmallShape});

        ASSERT_EQ(result.outcome, ChunkOutcome::Grown) << seed;
        EXPECT_TRUE(climbable(result, roofOf())) << seed;
    }
}

TEST(ChunkClimbTest, Grow_LeavesAWayUpAroundTheCubesTheArtistPainted)
{
    const auto hintVoxels = voxelsOf({
        VoxelCell{.x = 2, .y = 4, .z = 2, .kind = Kind::Normal},
        VoxelCell{.x = 3, .y = 4, .z = 2, .kind = Kind::Normal},
        VoxelCell{.x = 2, .y = 4, .z = 3, .kind = Kind::Normal}});

    const auto result = growChunk(
        city(),
        ChunkRequest{
            .seed = 3, .shape = kSmallShape, .hintVoxels = hintVoxels});

    ASSERT_EQ(result.outcome, ChunkOutcome::Grown);
    EXPECT_TRUE(climbable(result, roofOf()));
}

TEST(ChunkClimbTest, Grow_LeavesAStreetToSetOutFromEvenWithNoWayUp)
{
    const auto result = growChunk(
        city(), ChunkRequest{.seed = 1, .shape = kSmallShape, .ways = 0});

    ASSERT_EQ(result.outcome, ChunkOutcome::Grown);

    const FakeChunkWalkGraph graph(kSmallShape, result.cubeVoxels);

    EXPECT_FALSE(graph.streets().empty());
}
