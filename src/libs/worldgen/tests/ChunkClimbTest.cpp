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
using antwika::pathfinding::getPathBetween;
using antwika::voxel::Kind;
using antwika::voxel::VoxelCell;
using antwika::voxel::voxelsOf;
using antwika::voxel::VoxelPosition;
using antwika::worldgen::ChunkOutcome;
using antwika::worldgen::ChunkRequest;
using antwika::worldgen::ChunkResult;
using antwika::worldgen::ChunkShape;
using antwika::worldgen::getCityRuleset;
using antwika::worldgen::CompiledRuleset;
using antwika::worldgen::getCubeCount;
using antwika::worldgen::getGrowChunk;
using antwika::worldgen::fakes::FakeChunkWalkGraph;

namespace
{
    constexpr ChunkShape kSmallShape{.width = 6, .depth = 6, .height = 16};

    const CompiledRuleset &getCity()
    {
        static const CompiledRuleset compiledRuleset(getCityRuleset());

        return compiledRuleset;
    }

    [[nodiscard]] std::int32_t roofOf()
    {
        return antwika::worldgen::detail::getHighestTerrace(getCity(), kSmallShape);
    }

    [[nodiscard]] bool isClimbable(
        const ChunkResult &result, const std::int32_t roof)
    {
        const FakeChunkWalkGraph graph(kSmallShape, result.cubeVoxels);
        const auto cap = static_cast<std::uint64_t>(getCubeCount(kSmallShape));

        for (const GridPos street : graph.getStreets())
        {
            for (std::int32_t x = 0; x < kSmallShape.width; ++x)
            {
                for (std::int32_t z = 0; z < kSmallShape.depth; ++z)
                {
                    const GridPos topPos{.x = x, .y = roof, .z = z};

                    if (graph.isRoomy(topPos)
                        && getPathBetween(graph, street, topPos, cap).has_value())
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
            getGrowChunk(getCity(), ChunkRequest{.seed = seed, .shape = kSmallShape});

        ASSERT_EQ(result.outcome, ChunkOutcome::Grown) << seed;
        EXPECT_TRUE(isClimbable(result, roofOf())) << seed;
    }
}

TEST(ChunkClimbTest, Grow_LeavesAWayUpAroundTheCubesTheArtistPainted)
{
    const auto hintVoxels = voxelsOf({
        VoxelCell{.position = {.x = 2, .y = 4, .z = 2},
            .material = {.kind = Kind::Normal}},
        VoxelCell{.position = {.x = 3, .y = 4, .z = 2},
            .material = {.kind = Kind::Normal}},
        VoxelCell{.position = {.x = 2, .y = 4, .z = 3},
            .material = {.kind = Kind::Normal}}});

    const auto result = getGrowChunk(
        getCity(),
        ChunkRequest{
            .seed = 3, .shape = kSmallShape, .hintVoxels = hintVoxels});

    ASSERT_EQ(result.outcome, ChunkOutcome::Grown);
    EXPECT_TRUE(isClimbable(result, roofOf()));
}

TEST(ChunkClimbTest, Grow_LeavesAStreetToSetOutFromEvenWithNoWayUp)
{
    const auto result = getGrowChunk(
        getCity(), ChunkRequest{.seed = 1, .shape = kSmallShape, .ways = 0});

    ASSERT_EQ(result.outcome, ChunkOutcome::Grown);

    const FakeChunkWalkGraph graph(kSmallShape, result.cubeVoxels);

    EXPECT_FALSE(graph.getStreets().empty());
}
