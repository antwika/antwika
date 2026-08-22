#include <gtest/gtest.h>

#include <algorithm>
#include <vector>

#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/voxel/VoxelStairs.hpp>
#include <antwika/worldgen/ChunkShape.hpp>
#include <antwika/worldgen/Grow.hpp>

#include <antwika/solver/ChunkGrow.hpp>

using antwika::solver::growTrouble;
using antwika::solver::hintsFrom;
using antwika::solver::withChunkSpliced;
using antwika::voxel::cubeVoxels;
using antwika::voxel::Facing;
using antwika::voxel::kCubeSide;
using antwika::voxel::Kind;
using antwika::voxel::stepVectorFor;
using antwika::voxel::VoxelCell;
using antwika::voxel::VoxelMaterial;
using antwika::voxel::VoxelPosition;
using antwika::voxel::voxelsOf;
using antwika::voxel::Voxels;
using antwika::worldgen::ChunkOutcome;
using antwika::worldgen::ChunkResult;
using antwika::worldgen::ChunkShape;
using antwika::worldgen::chunkBox;

namespace
{
    constexpr ChunkShape kSmallShape{.width = 4, .depth = 4, .height = 4};

    [[nodiscard]] Voxels cubeAt(
        const VoxelPosition cubePosition,
        const Kind kind = Kind::Normal,
        const Facing facing = Facing::Any)
    {
        const VoxelPosition cornerPosition{
            .x = cubePosition.x * kCubeSide,
            .y = cubePosition.y * kCubeSide,
            .z = cubePosition.z * kCubeSide};

        auto cubeCells = cubeVoxels(cornerPosition, kind,
            stepVectorFor(facing));

        for (auto &[position, material] : cubeCells)
        {
            material.facing = kind == Kind::Ramp ? facing : Facing::Any;
        }

        return cubeCells;
    }
}

TEST(ChunkGrowTest, HintsFrom_TakesOneHintFromEveryCubeOfThePile)
{
    auto pile = cubeAt(VoxelPosition{.x = 1, .y = 1, .z = 1});
    const auto besideCube = cubeAt(VoxelPosition{.x = 2, .y = 1, .z = 1});
    pile.insert(besideCube.begin(), besideCube.end());

    const auto hints = hintsFrom(pile, kSmallShape, VoxelPosition{});

    ASSERT_EQ(hints.size(), 2U);
    EXPECT_TRUE(hints.contains(VoxelPosition{.x = 1, .y = 1, .z = 1}));
    EXPECT_TRUE(hints.contains(VoxelPosition{.x = 2, .y = 1, .z = 1}));
}

TEST(ChunkGrowTest, HintsFrom_ReadsTheKindAndTheFacingOfTheCubeAsItStands)
{
    const auto pile =
        cubeAt(VoxelPosition{.x = 1, .y = 1, .z = 1}, Kind::Ramp, Facing::East);

    const auto hints = hintsFrom(pile, kSmallShape, VoxelPosition{});

    ASSERT_EQ(hints.size(), 1U);
    EXPECT_EQ(hints.begin()->second.kind, Kind::Ramp);
    EXPECT_EQ(hints.begin()->second.facing, Facing::East);
}

TEST(ChunkGrowTest, HintsFrom_LeavesOutACubeOutsideTheChunk)
{
    const auto pile = cubeAt(VoxelPosition{.x = 9, .y = 1, .z = 1});

    EXPECT_TRUE(hintsFrom(pile, kSmallShape, VoxelPosition{}).empty());
}

TEST(ChunkGrowTest, HintsFrom_TakesNoHintFromAnEmptyCube)
{
    EXPECT_TRUE(hintsFrom({}, kSmallShape, VoxelPosition{}).empty());
}

TEST(ChunkGrowTest, HintsFrom_NamesEveryHintWhereTheChunkStandsInTheWorld)
{
    constexpr VoxelPosition originPointPosition{.x = 5, .y = -2, .z = 3};
    const auto pile = cubeAt(VoxelPosition{.x = 6, .y = -1, .z = 4});

    const auto hints = hintsFrom(pile, kSmallShape, originPointPosition);

    ASSERT_EQ(hints.size(), 1U);
    EXPECT_TRUE(hints.contains(VoxelPosition{.x = 6, .y = -1, .z = 4}));
}

TEST(ChunkGrowTest, WithChunkSpliced_TakesOutEveryVoxelTheChunkCovers)
{
    const auto pile = cubeAt(VoxelPosition{.x = 1, .y = 1, .z = 1});
    const auto box = chunkBox(kSmallShape, VoxelPosition{});

    const auto splicedPile = withChunkSpliced(pile, box, {});

    EXPECT_TRUE(splicedPile.empty());
}

TEST(ChunkGrowTest, WithChunkSpliced_LeavesEveryVoxelBesideTheChunkWhereItStood)
{
    const auto outsideCube = cubeAt(VoxelPosition{.x = 9, .y = 1, .z = 1});
    auto pile = outsideCube;
    const auto insideCube = cubeAt(VoxelPosition{.x = 1, .y = 1, .z = 1});
    pile.insert(insideCube.begin(), insideCube.end());

    const auto splicedPile =
        withChunkSpliced(pile, chunkBox(kSmallShape, VoxelPosition{}), {});

    EXPECT_EQ(splicedPile, outsideCube);
}

TEST(ChunkGrowTest, WithChunkSpliced_StandsEveryCubeGrown)
{
    const auto grownCube = cubeAt(VoxelPosition{.x = 2, .y = 2, .z = 2});

    const auto splicedPile =
        withChunkSpliced({}, chunkBox(kSmallShape, VoxelPosition{}), grownCube);

    EXPECT_EQ(splicedPile, grownCube);
}

TEST(ChunkGrowTest, GrowTrouble_TellsGivingUpApartFromCannotBeBuilt)
{
    const auto gaveUp =
        growTrouble(ChunkResult{.outcome = ChunkOutcome::LimitExceeded});
    const auto never =
        growTrouble(ChunkResult{.outcome = ChunkOutcome::Unsatisfiable});

    EXPECT_NE(gaveUp, never);
    EXPECT_FALSE(gaveUp.empty());
    EXPECT_FALSE(never.empty());
}

TEST(ChunkGrowTest, GrowTrouble_SaysSomethingOfEveryOutcome)
{
    for (const ChunkOutcome outcome :
         {ChunkOutcome::Grown,
          ChunkOutcome::HintOutside,
          ChunkOutcome::HintUnknown,
          ChunkOutcome::HintsConflict,
          ChunkOutcome::NoWayUp,
          ChunkOutcome::Unsatisfiable,
          ChunkOutcome::LimitExceeded})
    {
        EXPECT_FALSE(growTrouble(ChunkResult{.outcome = outcome}).empty());
    }
}
