#include <gtest/gtest.h>

#include <algorithm>
#include <vector>

#include <antwika/rng/SplitMix64Rng.hpp>
#include <antwika/voxel/VoxelCell.hpp>
#include <antwika/worldgen/ChunkShape.hpp>
#include <antwika/worldgen/CityRuleset.hpp>
#include <antwika/worldgen/Grow.hpp>
#include <antwika/worldgen/Ruleset.hpp>
#include <antwika/worldgen/WorldgenError.hpp>
#include <antwika/worldgen/ruleset/CompiledRuleset.hpp>

using antwika::rng::SplitMix64Rng;
using antwika::voxel::Facing;
using antwika::voxel::Kind;
using antwika::voxel::VoxelCell;
using antwika::worldgen::ChunkOutcome;
using antwika::worldgen::ChunkRequest;
using antwika::worldgen::ChunkResult;
using antwika::worldgen::ChunkShape;
using antwika::worldgen::CityPiece;
using antwika::worldgen::cityRuleset;
using antwika::worldgen::CompiledRuleset;
using antwika::worldgen::District;
using antwika::worldgen::growChunk;
using antwika::worldgen::maskOf;
using antwika::worldgen::Prototype;
using antwika::worldgen::Role;
using antwika::worldgen::Ruleset;
using antwika::worldgen::Socket;

namespace
{
    constexpr ChunkShape kSmallShape{.width = 6, .depth = 6, .height = 16};

    const CompiledRuleset &city()
    {
        static const CompiledRuleset compiledRuleset(cityRuleset());

        return compiledRuleset;
    }

    [[nodiscard]] const VoxelCell *found(
        const ChunkResult &result, const VoxelCell cubeCell)
    {
        const auto foundCell = std::ranges::find(result.cubeCells, cubeCell);

        return foundCell == result.cubeCells.end() ? nullptr : &*foundCell;
    }
}

TEST(GrowChunkTest, Grow_RaisesABlockFromNothingAtAll)
{
    const auto result =
        growChunk(city(), ChunkRequest{.seed = 1, .shape = kSmallShape});

    ASSERT_EQ(result.outcome, ChunkOutcome::Grown);
    EXPECT_FALSE(result.cubeCells.empty());
    EXPECT_TRUE(result.culpritCells.empty());
}

TEST(GrowChunkTest, Grow_GivesTheSameBlockTwiceFromOneSeed)
{
    const ChunkRequest request{.seed = 7, .shape = kSmallShape};

    EXPECT_EQ(growChunk(city(), request), growChunk(city(), request));
}

TEST(GrowChunkTest, Grow_GivesADifferentBlockFromADifferentSeed)
{
    const auto one =
        growChunk(city(), ChunkRequest{.seed = 1, .shape = kSmallShape});
    const auto otherChunk =
        growChunk(city(), ChunkRequest{.seed = 2, .shape = kSmallShape});

    ASSERT_EQ(one.outcome, ChunkOutcome::Grown);
    ASSERT_EQ(otherChunk.outcome, ChunkOutcome::Grown);
    EXPECT_NE(one.cubeCells, otherChunk.cubeCells);
}

TEST(GrowChunkTest, Grow_DrawsItsWaysAndItsFillFromSeparateStreams)
{
    const ChunkRequest request{.seed = 3, .shape = kSmallShape};

    SplitMix64Rng waysRng(11);
    SplitMix64Rng fillRng(22);
    const auto one = growChunk(city(), request, waysRng, fillRng);

    SplitMix64Rng sameWaysRng(11);
    SplitMix64Rng otherFillRng(33);
    const auto otherChunk = growChunk(city(), request, sameWaysRng,
        otherFillRng);

    ASSERT_EQ(one.outcome, ChunkOutcome::Grown);
    ASSERT_EQ(otherChunk.outcome, ChunkOutcome::Grown);
    EXPECT_NE(one.cubeCells, otherChunk.cubeCells);
}

TEST(GrowChunkTest, Grow_StandsEveryCubeAHintAsked)
{
    const std::vector<VoxelCell> hintCells{
        VoxelCell{.x = 2, .y = 5, .z = 2, .kind = Kind::Normal},
        VoxelCell{.x = 3, .y = 5, .z = 2, .kind = Kind::Normal}};

    const auto result = growChunk(
        city(),
        ChunkRequest{.seed = 5, .shape = kSmallShape, .hintCells = hintCells});

    ASSERT_EQ(result.outcome, ChunkOutcome::Grown);

    for (const VoxelCell hint : hintCells)
    {
        const auto *foundCell = found(result, hint);

        ASSERT_NE(foundCell, nullptr);
        EXPECT_EQ(foundCell->kind, hint.kind);
    }
}

TEST(GrowChunkTest, Grow_StandsAStairTheWayTheArtistPaintedIt)
{
    const std::vector<VoxelCell> hintCells{
        VoxelCell{
            .x = 2,
            .y = 5,
            .z = 2,
            .kind = Kind::Ramp,
            .facing = Facing::East}};

    const auto result = growChunk(
        city(),
        ChunkRequest{.seed = 4, .shape = kSmallShape, .hintCells = hintCells});

    ASSERT_EQ(result.outcome, ChunkOutcome::Grown);

    const auto *foundCell = found(result, hintCells.front());

    ASSERT_NE(foundCell, nullptr);
    EXPECT_EQ(foundCell->kind, Kind::Ramp);
    EXPECT_EQ(foundCell->facing, Facing::East);
}

TEST(GrowChunkTest, Grow_NamesAHintOutsideTheChunk)
{
    const auto result = growChunk(
        city(),
        ChunkRequest{
            .shape = kSmallShape,
            .hintCells = {VoxelCell{.x = 99, .y = 1, .z = 1}}});

    EXPECT_EQ(result.outcome, ChunkOutcome::HintOutside);
    ASSERT_EQ(result.culpritCells.size(), 1U);
    EXPECT_EQ(result.culpritCells.front().x, 99);
}

TEST(GrowChunkTest, Grow_NamesAHintNoPieceIsLaidOutAs)
{
    auto ruleset = cityRuleset();
    ruleset.prototypes.erase(
        ruleset.prototypes.begin()
        + static_cast<std::ptrdiff_t>(
            antwika::worldgen::indexOf(CityPiece::Cistern)));

    for (auto &district : ruleset.districts)
    {
        district.desire.pop_back();
    }

    const CompiledRuleset compiledRuleset(ruleset);

    const auto result = growChunk(
        compiledRuleset,
        ChunkRequest{
            .shape = kSmallShape,
            .hintCells = {
                VoxelCell{
                    .x = 2, .y = 5, .z = 2, .kind = Kind::Water}}});

    EXPECT_EQ(result.outcome, ChunkOutcome::HintUnknown);
}

TEST(GrowChunkTest, Grow_NamesAHintTheDistrictItStandsInRefuses)
{
    const auto result = growChunk(
        city(),
        ChunkRequest{
            .shape = kSmallShape,
            .hintCells = {
                VoxelCell{.x = 2, .y = 13, .z = 2, .kind = Kind::Water}}});

    EXPECT_EQ(result.outcome, ChunkOutcome::HintsConflict);
    ASSERT_EQ(result.culpritCells.size(), 1U);
}

TEST(GrowChunkTest, Grow_NamesBothCubesWhereTwoHintsStandAgainstOneAnother)
{
    const auto result = growChunk(
        city(),
        ChunkRequest{
            .shape = kSmallShape,
            .hintCells =
                {VoxelCell{
                     .x = 2,
                     .y = 1,
                     .z = 2,
                     .kind = Kind::Ramp,
                     .facing = Facing::East},
                 VoxelCell{.x = 3, .y = 1, .z = 2, .kind = Kind::Water}},
            .ways = 0});

    EXPECT_EQ(result.outcome, ChunkOutcome::HintsConflict);
    EXPECT_EQ(result.culpritCells.size(), 2U);
}

TEST(GrowChunkTest, Grow_NamesTheThinnestCubesWhereItGivesUp)
{
    const auto result = growChunk(
        city(),
        ChunkRequest{.seed = 9, .shape = kSmallShape, .maxSteps = 1});

    ASSERT_EQ(result.outcome, ChunkOutcome::LimitExceeded);
    EXPECT_FALSE(result.culpritCells.empty());
    EXPECT_LE(
        result.culpritCells.size(), antwika::worldgen::kMaxReportedCulprits);
}

TEST(GrowChunkTest, Grow_TellsGivingUpApartFromCannotBeBuilt)
{
    const auto gaveUp = growChunk(
        city(), ChunkRequest{.seed = 9, .shape = kSmallShape, .maxSteps = 1});

    EXPECT_EQ(gaveUp.outcome, ChunkOutcome::LimitExceeded);
    EXPECT_NE(gaveUp.outcome, ChunkOutcome::Unsatisfiable);
}

TEST(GrowChunkTest, Grow_GrowsABlockWithNoWayUpWhereNoneWasAsked)
{
    const auto result = growChunk(
        city(), ChunkRequest{.seed = 6, .shape = kSmallShape, .ways = 0});

    ASSERT_EQ(result.outcome, ChunkOutcome::Grown);
    EXPECT_FALSE(result.cubeCells.empty());
}

TEST(GrowChunkTest, Grow_NamesEveryCubeInTheWorldsOwnCubes)
{
    constexpr VoxelCell originPointCell{.x = 10, .y = -4, .z = 7};

    const auto result = growChunk(
        city(),
        ChunkRequest{
            .seed = 8,
            .shape = kSmallShape,
            .originCell = originPointCell});

    ASSERT_EQ(result.outcome, ChunkOutcome::Grown);

    for (const VoxelCell cube : result.cubeCells)
    {
        EXPECT_GE(cube.x, originPointCell.x);
        EXPECT_LT(cube.x, originPointCell.x + kSmallShape.width);
        EXPECT_GE(cube.y, originPointCell.y);
        EXPECT_LT(cube.y, originPointCell.y + kSmallShape.height);
        EXPECT_GE(cube.z, originPointCell.z);
        EXPECT_LT(cube.z, originPointCell.z + kSmallShape.depth);
    }
}

TEST(GrowChunkTest, Grow_LeavesOutTheAirRatherThanStandingIt)
{
    const auto result =
        growChunk(city(), ChunkRequest{.seed = 2, .shape = kSmallShape});

    ASSERT_EQ(result.outcome, ChunkOutcome::Grown);
    EXPECT_LT(result.cubeCells.size(), cubeCount(kSmallShape));
}

TEST(GrowChunkTest, Grow_LaysTheGroundBeforeAnythingStandsOnIt)
{
    const auto result =
        growChunk(city(), ChunkRequest{.seed = 12, .shape = kSmallShape});

    ASSERT_EQ(result.outcome, ChunkOutcome::Grown);

    for (std::size_t index = 1; index < result.cubeCells.size(); ++index)
    {
        EXPECT_LE(result.cubeCells[index - 1].y, result.cubeCells[index].y);
    }

    const auto onTheFloor = std::ranges::count_if(
        result.cubeCells,
        [](const VoxelCell cube) { return cube.y == 0; });

    EXPECT_GT(onTheFloor, (kSmallShape.width * kSmallShape.depth) / 2);
}

TEST(GrowChunkTest, Grow_TurnsAwayAShapeWithNoSideToIt)
{
    EXPECT_THROW(
        (void)growChunk(
            city(), ChunkRequest{.shape = ChunkShape{.height = 0}}),
        antwika::worldgen::WorldgenError);
}

TEST(GrowChunkTest, Grow_NamesTheBlockUnclimbableWhereEveryCubeIsSettled)
{
    constexpr ChunkShape tinyShape{.width = 2, .depth = 2, .height = 2};

    std::vector<VoxelCell> hintCells;
    for (std::int32_t y = 0; y < tinyShape.height; ++y)
    {
        for (std::int32_t z = 0; z < tinyShape.depth; ++z)
        {
            for (std::int32_t x = 0; x < tinyShape.width; ++x)
            {
                hintCells.push_back(
                    VoxelCell{.x = x, .y = y, .z = z, .kind = Kind::Normal});
            }
        }
    }

    const auto result = growChunk(
        city(), ChunkRequest{
            .shape = tinyShape,
            .hintCells = hintCells,
            .ways = 1});

    EXPECT_EQ(result.outcome, ChunkOutcome::NoWayUp);
    EXPECT_EQ(result.culpritCells.size(), 1U);
}

TEST(GrowChunkTest, Grow_NamesHintsThatFallOutOnlyOnceTheyHaveSpread)
{
    constexpr ChunkShape shape{.width = 4, .depth = 4, .height = 16};

    const auto result = growChunk(
        city(),
        ChunkRequest{
            .shape = shape,
            .hintCells =
                {VoxelCell{
                     .x = 1,
                     .y = 1,
                     .z = 1,
                     .kind = Kind::Ramp,
                     .facing = Facing::East},
                 VoxelCell{.x = 2, .y = 2, .z = 1, .kind = Kind::Water}},
            .ways = 0});

    EXPECT_EQ(result.outcome, ChunkOutcome::HintsConflict);
    EXPECT_FALSE(result.culpritCells.empty());
}

TEST(GrowChunkTest, Grow_NamesABlockNoDistrictCanBeStackedInto)
{
    Ruleset ruleset;
    ruleset.prototypes = {
        Prototype{
            .name = "sky",
            .air = true,
            .sockets =
                {Socket::OpenSide,
                 Socket::OpenSide,
                 Socket::Sky,
                 Socket::Floats,
                 Socket::OpenSide,
                 Socket::OpenSide},
            .roles = static_cast<std::uint8_t>(
                maskOf(Role::Room)
                | maskOf(Role::Perch)
                | maskOf(Role::Climb)
                | maskOf(Role::Step))},
        Prototype{
            .name = "stone",
            .sockets =
                {Socket::Facade,
                 Socket::Facade,
                 Socket::Carries,
                 Socket::Rests,
                 Socket::Facade,
                 Socket::Facade},
            .roles = static_cast<std::uint8_t>(
                maskOf(Role::Bear)
                | maskOf(Role::Land))}};
    ruleset.districts = {
        District{
            .name = "sky below", .untilShare = 50, .desire = {1, 0}},
        District{
            .name = "stone above", .untilShare = 100, .desire = {0, 1}}};

    const CompiledRuleset compiledRuleset(ruleset);

    const auto result = growChunk(
        compiledRuleset,
        ChunkRequest{.shape = ChunkShape{.width = 2, .depth = 2, .height = 2}});

    EXPECT_EQ(result.outcome, ChunkOutcome::Unsatisfiable);
    EXPECT_FALSE(result.culpritCells.empty());
}
