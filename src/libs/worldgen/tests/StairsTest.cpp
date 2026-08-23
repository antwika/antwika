#include <gtest/gtest.h>

#include <algorithm>
#include <numeric>
#include <ranges>
#include <vector>

#include <antwika/rng/SplitMix64Rng.hpp>
#include <antwika/rng/fakes/FakeRng.hpp>
#include <antwika/voxel/VoxelCell.hpp>
#include <antwika/wfc/Domain.hpp>
#include <antwika/worldgen/ChunkShape.hpp>
#include <antwika/worldgen/CityRuleset.hpp>
#include <antwika/worldgen/Grow.hpp>
#include <antwika/worldgen/Ruleset.hpp>
#include <antwika/worldgen/ruleset/CompiledRuleset.hpp>

#include "Lattice.hpp"
#include "Stairs.hpp"

using antwika::rng::SplitMix64Rng;
using antwika::rng::fakes::FakeRng;
using antwika::voxel::Kind;
using antwika::voxel::VoxelCell;
using antwika::voxel::VoxelPosition;
using antwika::wfc::Domain;
using antwika::worldgen::cellOf;
using antwika::worldgen::ChunkShape;
using antwika::worldgen::getCityRuleset;
using antwika::worldgen::CompiledRuleset;
using antwika::worldgen::cubeAt;
using antwika::worldgen::getCubeCount;
using antwika::worldgen::Role;
using antwika::worldgen::detail::Board;
using antwika::worldgen::detail::fits;
using antwika::worldgen::detail::getHighestTerrace;
using antwika::worldgen::detail::layWays;
using antwika::worldgen::detail::settle;
using antwika::worldgen::detail::getWalkSteps;

namespace
{
    constexpr ChunkShape kSmallShape{.width = 6, .depth = 6, .height = 16};

    const CompiledRuleset &getCity()
    {
        static const CompiledRuleset compiledRuleset(getCityRuleset());

        return compiledRuleset;
    }

    std::vector<Domain> getFreshWave(const ChunkShape shape)
    {
        const auto &compiledRuleset = getCity();
        std::vector<Domain> waveDomains(
            getCubeCount(shape),
            Domain(compiledRuleset.getSize()));

        for (std::size_t cell = 0; cell < waveDomains.size(); ++cell)
        {
            const auto cube = cubeAt(shape, cell);
            const auto desire =
                compiledRuleset.desireIn(
                    compiledRuleset.districtOf(shape, cube.y));

            for (std::size_t which = 0; which < compiledRuleset.getSize(); ++which)
            {
                if (desire[which] == 0)
                {
                    waveDomains[cell].remove(which);
                }
            }
        }

        return waveDomains;
    }
}

TEST(StairsTest, HighestTerrace_StopsBelowTheSkyWhereNothingBears)
{
    const std::int32_t roof = getHighestTerrace(getCity(), kSmallShape);

    EXPECT_GT(roof, 0);
    EXPECT_LT(roof, kSmallShape.height - 1);
}

TEST(StairsTest, LayWays_LaysNothingWhereNoneWasAsked)
{
    auto wave = getFreshWave(kSmallShape);
    Board board(wave);
    SplitMix64Rng rng(1);

    const auto laidWays = layWays(getCity(), kSmallShape, board, 0, rng);

    EXPECT_TRUE(laidWays.climbed);
    EXPECT_TRUE(laidWays.landings.empty());
    EXPECT_EQ(board.getMarkStep(), 0U);
}

TEST(StairsTest, LayWays_ClimbsFromTheStreetToTheHighestTerrace)
{
    auto wave = getFreshWave(kSmallShape);
    Board board(wave);
    SplitMix64Rng rng(2);

    const auto laidWays = layWays(getCity(), kSmallShape, board, 1, rng);

    ASSERT_TRUE(laidWays.climbed);
    ASSERT_FALSE(laidWays.landings.empty());

    const std::int32_t roof = getHighestTerrace(getCity(), kSmallShape);
    const auto highest = std::ranges::max(
        laidWays.landings
        | std::views::transform(
            [](const std::size_t cell)
            { return cubeAt(kSmallShape, cell).y; }));

    EXPECT_GE(highest, roof);
}

TEST(StairsTest, LayWays_LeavesEveryCubeItWalkedStandableOrClimbable)
{
    auto wave = getFreshWave(kSmallShape);
    Board board(wave);
    SplitMix64Rng rng(3);

    const auto laidWays = layWays(getCity(), kSmallShape, board, 3, rng);

    ASSERT_TRUE(laidWays.climbed);

    for (const std::size_t cell : laidWays.landings)
    {
        EXPECT_TRUE(fits(board, cell, getCity().getWearing(Role::Perch)));
    }
}

TEST(StairsTest, LayWays_LeavesEveryCubeSomethingItStillMayBe)
{
    auto wave = getFreshWave(kSmallShape);
    Board board(wave);
    SplitMix64Rng rng(4);

    ASSERT_TRUE(layWays(getCity(), kSmallShape, board, 5, rng).climbed);

    for (const auto &domain : wave)
    {
        EXPECT_FALSE(domain.isEmpty());
    }
}

TEST(StairsTest, LayWays_StartsEveryBranchFromALandingAlreadyLaid)
{
    auto wave = getFreshWave(kSmallShape);
    Board board(wave);
    SplitMix64Rng rng(5);

    const auto one = layWays(getCity(), kSmallShape, board, 1, rng);

    auto otherWave = getFreshWave(kSmallShape);
    Board otherBoard(otherWave);
    SplitMix64Rng sameRng(5);

    const auto many = layWays(getCity(), kSmallShape, otherBoard, 4, sameRng);

    ASSERT_TRUE(one.climbed);
    ASSERT_TRUE(many.climbed);
    EXPECT_GT(many.landings.size(), one.landings.size());
}

TEST(StairsTest, LayWays_NeverWalksTheSameCubeTwice)
{
    auto wave = getFreshWave(kSmallShape);
    Board board(wave);
    SplitMix64Rng rng(6);

    const auto laidWays = layWays(getCity(), kSmallShape, board, 4, rng);

    ASSERT_TRUE(laidWays.climbed);

    auto sortedLandings = laidWays.landings;
    std::ranges::sort(sortedLandings);
    const auto twice = std::ranges::unique(sortedLandings);

    EXPECT_EQ(twice.begin(), sortedLandings.end());
}

TEST(StairsTest, LayWays_TakesTheSameWayTwiceFromOneSeed)
{
    auto wave = getFreshWave(kSmallShape);
    Board board(wave);
    SplitMix64Rng rng(7);
    const auto one = layWays(getCity(), kSmallShape, board, 3, rng);

    auto otherWave = getFreshWave(kSmallShape);
    Board otherBoard(otherWave);
    SplitMix64Rng sameRng(7);
    const auto twice = layWays(getCity(), kSmallShape, otherBoard, 3, sameRng);

    EXPECT_EQ(one.landings, twice.landings);
}

TEST(StairsTest, LayWays_GivesUpWhereEveryWayIsWalledOff)
{
    auto wave = getFreshWave(kSmallShape);
    Board board(wave);

    for (std::size_t cell = 0; cell < wave.size(); ++cell)
    {
        if (cubeAt(kSmallShape, cell).y != 0)
        {
            board.hold(cell, getCity().getWearing(Role::Bear));
        }
    }

    SplitMix64Rng rng(8);
    const auto laidWays = layWays(getCity(), kSmallShape, board, 1, rng);

    EXPECT_FALSE(laidWays.climbed);
}

TEST(StairsTest, LayWays_PutsBackEveryCubeOfAWayItGivesUpOn)
{
    auto wave = getFreshWave(kSmallShape);
    Board board(wave);

    for (std::size_t cell = 0; cell < wave.size(); ++cell)
    {
        if (cubeAt(kSmallShape, cell).y != 0)
        {
            board.hold(cell, getCity().getWearing(Role::Bear));
        }
    }

    const std::size_t mark = board.getMarkStep();
    SplitMix64Rng rng(9);

    ASSERT_FALSE(layWays(getCity(), kSmallShape, board, 1, rng).climbed);
    EXPECT_EQ(board.getMarkStep(), mark);
}

TEST(StairsTest, WalkSteps_GrowsWithEverySideOfTheChunk)
{
    EXPECT_GT(
        getWalkSteps(ChunkShape{.width = 8, .depth = 8, .height = 32}),
        getWalkSteps(ChunkShape{.width = 4, .depth = 4, .height = 8}));
}

TEST(StairsTest, Settle_TurnsAwayAWaveWithACubeThatCanBeNothing)
{
    auto wave = getFreshWave(kSmallShape);
    Board board(wave);

    const auto cube = VoxelPosition{.x = 2, .y = 5, .z = 2};
    const auto cell = cellOf(kSmallShape, cube);

    board.hold(cell, getCity().getMatching(Kind::Water, antwika::voxel::Facing::Any));

    EXPECT_TRUE(wave[cell].isEmpty());
}

TEST(StairsTest, Board_PutsBackWhatItTook)
{
    auto wave = getFreshWave(kSmallShape);
    const auto beforeWave = wave;
    Board board(wave);

    const std::size_t mark = board.getMarkStep();
    board.hold(
        cellOf(kSmallShape, VoxelPosition{.x = 2, .y = 5, .z = 2}),
        getCity().getWearing(Role::Bear));

    EXPECT_NE(wave, beforeWave);

    board.rewind(mark);

    EXPECT_EQ(wave, beforeWave);
}

TEST(StairsTest, Settle_LeavesAWaveThatIsAlreadySettledAlone)
{
    auto wave = getFreshWave(kSmallShape);
    Board board(wave);

    std::vector<std::size_t> every(wave.size());
    std::iota(every.begin(), every.end(), std::size_t{0});

    ASSERT_TRUE(settle(getCity(), kSmallShape, board, every));

    const std::size_t mark = board.getMarkStep();

    EXPECT_TRUE(settle(getCity(), kSmallShape, board, every));
    EXPECT_EQ(board.getMarkStep(), mark);
}

TEST(StairsTest, HighestTerrace_StopsAtTheFootWhereNoDistrictWantsGround)
{
    auto ruleset = getCityRuleset();

    for (auto &district : ruleset.districts)
    {
        for (const std::size_t which :
             CompiledRuleset(getCityRuleset()).getWearing(Role::Bear))
        {
            district.desire[which] = 0;
        }
    }

    const CompiledRuleset compiledRuleset(ruleset);

    EXPECT_EQ(getHighestTerrace(compiledRuleset, kSmallShape), 0);
}

TEST(StairsTest, LayWays_LaysGroundUnderAStreetAboveTheFloor)
{
    auto wave = getFreshWave(kSmallShape);
    Board board(wave);

    for (std::size_t cell = 0; cell < wave.size(); ++cell)
    {
        if (cubeAt(kSmallShape, cell).y == 0)
        {
            board.hold(cell, getCity().getWearing(Role::Bear));
        }
    }

    SplitMix64Rng rng(11);
    const auto laidWays = layWays(getCity(), kSmallShape, board, 2, rng);

    ASSERT_TRUE(laidWays.climbed);

    for (const std::size_t cell : laidWays.landings)
    {
        EXPECT_GT(cubeAt(kSmallShape, cell).y, 0);
    }
}
