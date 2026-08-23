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
using antwika::voxel::VoxelMaterial;
using antwika::voxel::Voxels;
using antwika::voxel::VoxelPosition;
using antwika::voxel::voxelsOf;
using antwika::voxel::VoxelPosition;
using antwika::worldgen::ChunkOutcome;
using antwika::worldgen::ChunkRequest;
using antwika::worldgen::ChunkResult;
using antwika::worldgen::ChunkShape;
using antwika::worldgen::CityPiece;
using antwika::worldgen::getCityRuleset;
using antwika::worldgen::CompiledRuleset;
using antwika::worldgen::District;
using antwika::worldgen::getGrowChunk;
using antwika::worldgen::maskOf;
using antwika::worldgen::Prototype;
using antwika::worldgen::Role;
using antwika::worldgen::Ruleset;
using antwika::worldgen::Socket;

namespace
{
    constexpr ChunkShape kSmallShape{.width = 6, .depth = 6, .height = 16};

    const CompiledRuleset &getCity()
    {
        static const CompiledRuleset compiledRuleset(getCityRuleset());

        return compiledRuleset;
    }

    [[nodiscard]] const VoxelMaterial *getFoundMaterial(
        const ChunkResult &result, const VoxelPosition cubePosition)
    {
        const auto foundVoxel = result.cubeVoxels.find(cubePosition);

        return foundVoxel == result.cubeVoxels.end()
                   ? nullptr
                   : &foundVoxel->second;
    }
}

TEST(GrowChunkTest, Grow_RaisesABlockFromNothingAtAll)
{
    const auto result =
        getGrowChunk(getCity(), ChunkRequest{.seed = 1, .shape = kSmallShape});

    ASSERT_EQ(result.outcome, ChunkOutcome::Grown);
    EXPECT_FALSE(result.cubeVoxels.empty());
    EXPECT_TRUE(result.culpritPositions.empty());
}

TEST(GrowChunkTest, Grow_GivesTheSameBlockTwiceFromOneSeed)
{
    const ChunkRequest request{.seed = 7, .shape = kSmallShape};

    EXPECT_EQ(getGrowChunk(getCity(), request), getGrowChunk(getCity(), request));
}

TEST(GrowChunkTest, Grow_GivesADifferentBlockFromADifferentSeed)
{
    const auto one =
        getGrowChunk(getCity(), ChunkRequest{.seed = 1, .shape = kSmallShape});
    const auto otherChunk =
        getGrowChunk(getCity(), ChunkRequest{.seed = 2, .shape = kSmallShape});

    ASSERT_EQ(one.outcome, ChunkOutcome::Grown);
    ASSERT_EQ(otherChunk.outcome, ChunkOutcome::Grown);
    EXPECT_NE(one.cubeVoxels, otherChunk.cubeVoxels);
}

TEST(GrowChunkTest, Grow_DrawsItsWaysAndItsFillFromSeparateStreams)
{
    const ChunkRequest request{.seed = 3, .shape = kSmallShape};

    SplitMix64Rng waysRng(11);
    SplitMix64Rng fillRng(22);
    const auto one = getGrowChunk(getCity(), request, waysRng, fillRng);

    SplitMix64Rng sameWaysRng(11);
    SplitMix64Rng otherFillRng(33);
    const auto otherChunk = getGrowChunk(getCity(), request, sameWaysRng,
        otherFillRng);

    ASSERT_EQ(one.outcome, ChunkOutcome::Grown);
    ASSERT_EQ(otherChunk.outcome, ChunkOutcome::Grown);
    EXPECT_NE(one.cubeVoxels, otherChunk.cubeVoxels);
}

TEST(GrowChunkTest, Grow_StandsEveryCubeAHintAsked)
{
    const auto hintVoxels = voxelsOf({
        VoxelCell{.position = {.x = 2, .y = 5, .z = 2},
            .material = {.kind = Kind::Normal}},
        VoxelCell{.position = {.x = 3, .y = 5, .z = 2},
            .material = {.kind = Kind::Normal}}});

    const auto result = getGrowChunk(
        getCity(),
        ChunkRequest{.seed = 5, .shape = kSmallShape,
            .hintVoxels = hintVoxels});

    ASSERT_EQ(result.outcome, ChunkOutcome::Grown);

    for (const auto &[hintPosition, hintMaterial] : hintVoxels)
    {
        const auto *foundCell = getFoundMaterial(result, hintPosition);

        ASSERT_NE(foundCell, nullptr);
        EXPECT_EQ(foundCell->kind, hintMaterial.kind);
    }
}

TEST(GrowChunkTest, Grow_StandsAStairTheWayTheArtistPaintedIt)
{
    const auto hintVoxels = voxelsOf({
        VoxelCell{.position = {.x = 2, .y = 5, .z = 2},
            .material = {.kind = Kind::Ramp, .facing = Facing::East}}});

    const auto result = getGrowChunk(
        getCity(),
        ChunkRequest{.seed = 4, .shape = kSmallShape,
            .hintVoxels = hintVoxels});

    ASSERT_EQ(result.outcome, ChunkOutcome::Grown);

    const auto *foundCell = getFoundMaterial(result, hintVoxels.begin()->first);

    ASSERT_NE(foundCell, nullptr);
    EXPECT_EQ(foundCell->kind, Kind::Ramp);
    EXPECT_EQ(foundCell->facing, Facing::East);
}

TEST(GrowChunkTest, Grow_NamesAHintOutsideTheChunk)
{
    const auto result = getGrowChunk(
        getCity(),
        ChunkRequest{
            .shape = kSmallShape,
            .hintVoxels = voxelsOf({VoxelCell{.position = {.x = 99, .y = 1,
                .z = 1}}})});

    EXPECT_EQ(result.outcome, ChunkOutcome::HintOutside);
    ASSERT_EQ(result.culpritPositions.size(), 1U);
    EXPECT_EQ(result.culpritPositions.front().x, 99);
}

TEST(GrowChunkTest, Grow_NamesAHintNoPieceIsLaidOutAs)
{
    auto ruleset = getCityRuleset();
    ruleset.prototypes.erase(
        ruleset.prototypes.begin()
        + static_cast<std::ptrdiff_t>(
            antwika::worldgen::indexOf(CityPiece::Cistern)));

    for (auto &district : ruleset.districts)
    {
        district.desire.pop_back();
    }

    const CompiledRuleset compiledRuleset(ruleset);

    const auto result = getGrowChunk(
        compiledRuleset,
        ChunkRequest{
            .shape = kSmallShape,
            .hintVoxels = voxelsOf({
                VoxelCell{.position = {.x = 2, .y = 5, .z = 2},
                    .material = {.kind = Kind::Water}}})});

    EXPECT_EQ(result.outcome, ChunkOutcome::HintUnknown);
}

TEST(GrowChunkTest, Grow_NamesAHintTheDistrictItStandsInRefuses)
{
    const auto result = getGrowChunk(
        getCity(),
        ChunkRequest{
            .shape = kSmallShape,
            .hintVoxels = voxelsOf({
                VoxelCell{.position = {.x = 2, .y = 13, .z = 2},
                    .material = {.kind = Kind::Water}}})});

    EXPECT_EQ(result.outcome, ChunkOutcome::HintsConflict);
    ASSERT_EQ(result.culpritPositions.size(), 1U);
}

TEST(GrowChunkTest, Grow_NamesBothCubesWhereTwoHintsStandAgainstOneAnother)
{
    const auto result = getGrowChunk(
        getCity(),
        ChunkRequest{
            .shape = kSmallShape,
            .hintVoxels = voxelsOf(
                {VoxelCell{.position = {.x = 2, .y = 1, .z = 2},
                    .material = {.kind = Kind::Ramp, .facing = Facing::East}},
                 VoxelCell{.position = {.x = 3, .y = 1, .z = 2},
                     .material = {.kind = Kind::Water}}}),
            .ways = 0});

    EXPECT_EQ(result.outcome, ChunkOutcome::HintsConflict);
    EXPECT_EQ(result.culpritPositions.size(), 2U);
}

TEST(GrowChunkTest, Grow_NamesTheThinnestCubesWhereItGivesUp)
{
    const auto result = getGrowChunk(
        getCity(),
        ChunkRequest{.seed = 9, .shape = kSmallShape, .maxSteps = 1});

    ASSERT_EQ(result.outcome, ChunkOutcome::LimitExceeded);
    EXPECT_FALSE(result.culpritPositions.empty());
    EXPECT_LE(
        result.culpritPositions.size(
            ), antwika::worldgen::kMaxReportedCulprits);
}

TEST(GrowChunkTest, Grow_TellsGivingUpApartFromCannotBeBuilt)
{
    const auto gaveUp = getGrowChunk(
        getCity(), ChunkRequest{.seed = 9, .shape = kSmallShape, .maxSteps = 1});

    EXPECT_EQ(gaveUp.outcome, ChunkOutcome::LimitExceeded);
    EXPECT_NE(gaveUp.outcome, ChunkOutcome::Unsatisfiable);
}

TEST(GrowChunkTest, Grow_GrowsABlockWithNoWayUpWhereNoneWasAsked)
{
    const auto result = getGrowChunk(
        getCity(), ChunkRequest{.seed = 6, .shape = kSmallShape, .ways = 0});

    ASSERT_EQ(result.outcome, ChunkOutcome::Grown);
    EXPECT_FALSE(result.cubeVoxels.empty());
}

TEST(GrowChunkTest, Grow_NamesEveryCubeInTheWorldsOwnCubes)
{
    constexpr VoxelPosition originPointPosition{.x = 10, .y = -4, .z = 7};

    const auto result = getGrowChunk(
        getCity(),
        ChunkRequest{
            .seed = 8,
            .shape = kSmallShape,
            .originPosition = originPointPosition});

    ASSERT_EQ(result.outcome, ChunkOutcome::Grown);

    for (const auto &[cube, material] : result.cubeVoxels)
    {
        EXPECT_GE(cube.x, originPointPosition.x);
        EXPECT_LT(cube.x, originPointPosition.x + kSmallShape.width);
        EXPECT_GE(cube.y, originPointPosition.y);
        EXPECT_LT(cube.y, originPointPosition.y + kSmallShape.height);
        EXPECT_GE(cube.z, originPointPosition.z);
        EXPECT_LT(cube.z, originPointPosition.z + kSmallShape.depth);
    }
}

TEST(GrowChunkTest, Grow_LeavesOutTheAirRatherThanStandingIt)
{
    const auto result =
        getGrowChunk(getCity(), ChunkRequest{.seed = 2, .shape = kSmallShape});

    ASSERT_EQ(result.outcome, ChunkOutcome::Grown);
    EXPECT_LT(result.cubeVoxels.size(), getCubeCount(kSmallShape));
}

TEST(GrowChunkTest, Grow_LaysTheGroundUnderMostOfTheBlock)
{
    const auto result =
        getGrowChunk(getCity(), ChunkRequest{.seed = 12, .shape = kSmallShape});

    ASSERT_EQ(result.outcome, ChunkOutcome::Grown);

    const auto onTheFloor = std::ranges::count_if(
        result.cubeVoxels,
        [](const auto &standing) { return standing.first.y == 0; });

    EXPECT_GT(onTheFloor, (kSmallShape.width * kSmallShape.depth) / 2);
}

TEST(GrowChunkTest, Grow_TurnsAwayAShapeWithNoSideToIt)
{
    EXPECT_THROW(
        (void)getGrowChunk(
            getCity(), ChunkRequest{.shape = ChunkShape{.height = 0}}),
        antwika::worldgen::WorldgenError);
}

TEST(GrowChunkTest, Grow_NamesTheBlockUnclimbableWhereEveryCubeIsSettled)
{
    constexpr ChunkShape tinyShape{.width = 2, .depth = 2, .height = 2};

    Voxels hintVoxels;
    for (std::int32_t y = 0; y < tinyShape.height; ++y)
    {
        for (std::int32_t z = 0; z < tinyShape.depth; ++z)
        {
            for (std::int32_t x = 0; x < tinyShape.width; ++x)
            {
                hintVoxels[VoxelPosition{.x = x, .y = y, .z = z}] =
                    VoxelMaterial{.kind = Kind::Normal};
            }
        }
    }

    const auto result = getGrowChunk(
        getCity(), ChunkRequest{
            .shape = tinyShape,
            .hintVoxels = hintVoxels,
            .ways = 1});

    EXPECT_EQ(result.outcome, ChunkOutcome::NoWayUp);
    EXPECT_EQ(result.culpritPositions.size(), 1U);
}

TEST(GrowChunkTest, Grow_NamesHintsThatFallOutOnlyOnceTheyHaveSpread)
{
    constexpr ChunkShape shape{.width = 4, .depth = 4, .height = 16};

    const auto result = getGrowChunk(
        getCity(),
        ChunkRequest{
            .shape = shape,
            .hintVoxels = voxelsOf(
                {VoxelCell{.position = {.x = 1, .y = 1, .z = 1},
                    .material = {.kind = Kind::Ramp, .facing = Facing::East}},
                 VoxelCell{.position = {.x = 2, .y = 2, .z = 1},
                     .material = {.kind = Kind::Water}}}),
            .ways = 0});

    EXPECT_EQ(result.outcome, ChunkOutcome::HintsConflict);
    EXPECT_FALSE(result.culpritPositions.empty());
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

    const auto result = getGrowChunk(
        compiledRuleset,
        ChunkRequest{.shape = ChunkShape{.width = 2, .depth = 2, .height = 2}});

    EXPECT_EQ(result.outcome, ChunkOutcome::Unsatisfiable);
    EXPECT_FALSE(result.culpritPositions.empty());
}
