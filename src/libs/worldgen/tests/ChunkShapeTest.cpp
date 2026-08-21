#include <gtest/gtest.h>

#include <set>
#include <vector>

#include <antwika/voxel/VoxelCell.hpp>
#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/worldgen/ChunkShape.hpp>
#include <antwika/worldgen/WorldgenError.hpp>

using antwika::voxel::VoxelCell;
using antwika::worldgen::cellOf;
using antwika::worldgen::ChunkShape;
using antwika::worldgen::chunkBox;
using antwika::worldgen::cubeAt;
using antwika::worldgen::cubeCount;
using antwika::worldgen::holds;
using antwika::worldgen::within;
using antwika::worldgen::WorldgenError;

namespace
{
    constexpr ChunkShape kSmallShape{.width = 3, .depth = 4, .height = 5};
}

TEST(ChunkShapeTest, CubeCount_MultipliesTheThreeSides)
{
    EXPECT_EQ(cubeCount(kSmallShape), 60U);
}

TEST(ChunkShapeTest, CubeCount_TurnsAwayAShapeWithNoSideToIt)
{
    EXPECT_THROW((void)cubeCount(ChunkShape{.width = 0}), WorldgenError);
    EXPECT_THROW((void)cubeCount(ChunkShape{.depth = 0}), WorldgenError);
    EXPECT_THROW((void)cubeCount(ChunkShape{.height = 0}), WorldgenError);
}

TEST(ChunkShapeTest, CellOf_RunsBottomUpSoTiesBreakLow)
{
    for (std::int32_t lowLevel = 0; lowLevel < kSmallShape.height; ++lowLevel)
    {
        for (std::int32_t highLevel = lowLevel + 1;
             highLevel < kSmallShape.height;
             ++highLevel)
        {
            const auto underCell = cellOf(kSmallShape,
            VoxelCell{.y = lowLevel, .z = 3});
            const auto overCell = cellOf(kSmallShape,
                VoxelCell{.x = 2, .y = highLevel});

            EXPECT_LT(underCell, overCell);
        }
    }
}

TEST(ChunkShapeTest, CellOf_NumbersEveryCubeOnce)
{
    std::set<std::size_t> seenIndexes;

    for (std::int32_t y = 0; y < kSmallShape.height; ++y)
    {
        for (std::int32_t z = 0; z < kSmallShape.depth; ++z)
        {
            for (std::int32_t x = 0; x < kSmallShape.width; ++x)
            {
                seenIndexes.insert(
                    cellOf(kSmallShape, VoxelCell{.x = x, .y = y, .z = z}));
            }
        }
    }

    EXPECT_EQ(seenIndexes.size(), cubeCount(kSmallShape));
    EXPECT_EQ(*seenIndexes.begin(), 0U);
    EXPECT_EQ(*seenIndexes.rbegin(), cubeCount(kSmallShape) - 1);
}

TEST(ChunkShapeTest, CellOf_TurnsAwayACubeOutsideTheChunk)
{
    EXPECT_THROW((void)cellOf(kSmallShape, VoxelCell{.x = 3}), WorldgenError);
    EXPECT_THROW(
        (void)cellOf(ChunkShape{.width = 0}, VoxelCell{}), WorldgenError);
}

TEST(ChunkShapeTest, CubeAt_UndoesCellOf)
{
    for (std::size_t cell = 0; cell < cubeCount(kSmallShape); ++cell)
    {
        EXPECT_EQ(cellOf(kSmallShape, cubeAt(kSmallShape, cell)), cell);
    }
}

TEST(ChunkShapeTest, CubeAt_TurnsAwayACellTheChunkDoesNotHold)
{
    EXPECT_THROW(
        (void)cubeAt(kSmallShape, cubeCount(kSmallShape)),
        WorldgenError);
}

TEST(ChunkShapeTest, Within_TurnsAwayACubeOutsideTheShape)
{
    EXPECT_TRUE(within(kSmallShape, VoxelCell{.x = 2, .y = 4, .z = 3}));
    EXPECT_FALSE(within(kSmallShape, VoxelCell{.x = -1}));
    EXPECT_FALSE(within(kSmallShape, VoxelCell{.y = -1}));
    EXPECT_FALSE(within(kSmallShape, VoxelCell{.z = -1}));
    EXPECT_FALSE(within(kSmallShape, VoxelCell{.x = 3}));
    EXPECT_FALSE(within(kSmallShape, VoxelCell{.y = 5}));
    EXPECT_FALSE(within(kSmallShape, VoxelCell{.z = 4}));
}

TEST(ChunkShapeTest, ChunkBox_CoversEveryVoxelOfEveryCube)
{
    constexpr VoxelCell originPointCell{.x = 2, .y = -3, .z = 5};
    const auto box = chunkBox(kSmallShape, originPointCell);

    for (std::size_t cell = 0; cell < cubeCount(kSmallShape); ++cell)
    {
        const auto cube = cubeAt(kSmallShape, cell);
        const VoxelCell cornerCell{
            .x = (originPointCell.x + cube.x) * antwika::voxel::kCubeSide,
            .y = (originPointCell.y + cube.y) * antwika::voxel::kCubeSide,
            .z = (originPointCell.z + cube.z) * antwika::voxel::kCubeSide};

        for (const auto voxel : antwika::voxel::cubeCells(cornerCell))
        {
            EXPECT_TRUE(holds(box, voxel));
        }
    }
}

TEST(ChunkShapeTest, Holds_LeavesTheCubeBesideTheChunkAlone)
{
    const auto box = chunkBox(kSmallShape, VoxelCell{});

    EXPECT_TRUE(holds(box, VoxelCell{}));
    EXPECT_FALSE(holds(box, VoxelCell{.x = -1}));
    EXPECT_FALSE(holds(box, VoxelCell{.y = -1}));
    EXPECT_FALSE(holds(box, VoxelCell{.z = -1}));
    EXPECT_FALSE(holds(box, VoxelCell{.x = kSmallShape.width * 2}));
    EXPECT_FALSE(holds(box, VoxelCell{.y = kSmallShape.height * 2}));
    EXPECT_FALSE(holds(box, VoxelCell{.z = kSmallShape.depth * 2}));
}

TEST(ChunkShapeTest, ChunkBox_TurnsAwayAShapeWithNoSideToIt)
{
    EXPECT_THROW(
        (void)chunkBox(ChunkShape{.height = 0}, VoxelCell{}), WorldgenError);
}
