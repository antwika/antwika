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
using antwika::worldgen::ChunkOutcome;
using antwika::worldgen::ChunkResult;
using antwika::worldgen::ChunkShape;
using antwika::worldgen::chunkBox;

namespace
{
    constexpr ChunkShape kSmallShape{.width = 4, .depth = 4, .height = 4};

    [[nodiscard]] std::vector<VoxelCell> cubeAt(
        const VoxelCell cubeCell,
        const Kind kind = Kind::Normal,
        const Facing facing = Facing::Any)
    {
        const VoxelCell cornerCell{
            .x = cubeCell.x * kCubeSide,
            .y = cubeCell.y * kCubeSide,
            .z = cubeCell.z * kCubeSide};

        auto cubeCells = cubeVoxels(cornerCell, kind, stepVectorFor(facing));

        for (VoxelCell &voxel : cubeCells)
        {
            voxel.facing = kind == Kind::Ramp ? facing : Facing::Any;
        }

        return cubeCells;
    }
}

TEST(ChunkGrowTest, HintsFrom_TakesOneHintFromEveryCubeOfThePile)
{
    auto pile = cubeAt(VoxelCell{.x = 1, .y = 1, .z = 1});
    const auto besideCube = cubeAt(VoxelCell{.x = 2, .y = 1, .z = 1});
    pile.insert(pile.end(), besideCube.begin(), besideCube.end());

    const auto hints = hintsFrom(pile, kSmallShape, VoxelCell{});

    ASSERT_EQ(hints.size(), 2U);
    EXPECT_EQ(hints[0], (VoxelCell{.x = 1, .y = 1, .z = 1}));
    EXPECT_EQ(hints[1], (VoxelCell{.x = 2, .y = 1, .z = 1}));
}

TEST(ChunkGrowTest, HintsFrom_ReadsTheKindAndTheFacingOfTheCubeAsItStands)
{
    const auto pile =
        cubeAt(VoxelCell{.x = 1, .y = 1, .z = 1}, Kind::Ramp, Facing::East);

    const auto hints = hintsFrom(pile, kSmallShape, VoxelCell{});

    ASSERT_EQ(hints.size(), 1U);
    EXPECT_EQ(hints.front().kind, Kind::Ramp);
    EXPECT_EQ(hints.front().facing, Facing::East);
}

TEST(ChunkGrowTest, HintsFrom_LeavesOutACubeOutsideTheChunk)
{
    const auto pile = cubeAt(VoxelCell{.x = 9, .y = 1, .z = 1});

    EXPECT_TRUE(hintsFrom(pile, kSmallShape, VoxelCell{}).empty());
}

TEST(ChunkGrowTest, HintsFrom_TakesNoHintFromAnEmptyCube)
{
    EXPECT_TRUE(hintsFrom({}, kSmallShape, VoxelCell{}).empty());
}

TEST(ChunkGrowTest, HintsFrom_NamesEveryHintWhereTheChunkStandsInTheWorld)
{
    constexpr VoxelCell originPointCell{.x = 5, .y = -2, .z = 3};
    const auto pile = cubeAt(VoxelCell{.x = 6, .y = -1, .z = 4});

    const auto hints = hintsFrom(pile, kSmallShape, originPointCell);

    ASSERT_EQ(hints.size(), 1U);
    EXPECT_EQ(hints.front(), (VoxelCell{.x = 6, .y = -1, .z = 4}));
}

TEST(ChunkGrowTest, WithChunkSpliced_TakesOutEveryVoxelTheChunkCovers)
{
    const auto pile = cubeAt(VoxelCell{.x = 1, .y = 1, .z = 1});
    const auto box = chunkBox(kSmallShape, VoxelCell{});

    const auto splicedPile = withChunkSpliced(pile, box, {});

    EXPECT_TRUE(splicedPile.empty());
}

TEST(ChunkGrowTest, WithChunkSpliced_LeavesEveryVoxelBesideTheChunkWhereItStood)
{
    const auto outsideCube = cubeAt(VoxelCell{.x = 9, .y = 1, .z = 1});
    auto pile = outsideCube;
    const auto insideCube = cubeAt(VoxelCell{.x = 1, .y = 1, .z = 1});
    pile.insert(pile.end(), insideCube.begin(), insideCube.end());

    const auto splicedPile =
        withChunkSpliced(pile, chunkBox(kSmallShape, VoxelCell{}), {});

    EXPECT_EQ(splicedPile, outsideCube);
}

TEST(ChunkGrowTest, WithChunkSpliced_StandsEveryCubeGrown)
{
    const auto grownCube = cubeAt(VoxelCell{.x = 2, .y = 2, .z = 2});

    const auto splicedPile =
        withChunkSpliced({}, chunkBox(kSmallShape, VoxelCell{}), grownCube);

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
