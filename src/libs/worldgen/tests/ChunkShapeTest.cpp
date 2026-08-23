#include <gtest/gtest.h>

#include <set>
#include <vector>

#include <antwika/voxel/VoxelCell.hpp>
#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/worldgen/ChunkShape.hpp>
#include <antwika/worldgen/WorldgenError.hpp>

using antwika::voxel::VoxelPosition;
using antwika::voxel::VoxelPosition;
using antwika::worldgen::cellOf;
using antwika::worldgen::ChunkShape;
using antwika::worldgen::getChunkBox;
using antwika::worldgen::cubeAt;
using antwika::worldgen::getCubeCount;
using antwika::worldgen::holds;
using antwika::worldgen::isWithin;
using antwika::worldgen::WorldgenError;

namespace
{
    constexpr ChunkShape kSmallShape{.width = 3, .depth = 4, .height = 5};
}

TEST(ChunkShapeTest, CubeCount_MultipliesTheThreeSides)
{
    EXPECT_EQ(getCubeCount(kSmallShape), 60U);
}

TEST(ChunkShapeTest, CubeCount_TurnsAwayAShapeWithNoSideToIt)
{
    EXPECT_THROW((void)getCubeCount(ChunkShape{.width = 0}), WorldgenError);
    EXPECT_THROW((void)getCubeCount(ChunkShape{.depth = 0}), WorldgenError);
    EXPECT_THROW((void)getCubeCount(ChunkShape{.height = 0}), WorldgenError);
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
            VoxelPosition{.y = lowLevel, .z = 3});
            const auto overCell = cellOf(kSmallShape,
                VoxelPosition{.x = 2, .y = highLevel});

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
                    cellOf(kSmallShape, VoxelPosition{.x = x, .y = y, .z = z}));
            }
        }
    }

    EXPECT_EQ(seenIndexes.size(), getCubeCount(kSmallShape));
    EXPECT_EQ(*seenIndexes.begin(), 0U);
    EXPECT_EQ(*seenIndexes.rbegin(), getCubeCount(kSmallShape) - 1);
}

TEST(ChunkShapeTest, CellOf_TurnsAwayACubeOutsideTheChunk)
{
    EXPECT_THROW((void)cellOf(kSmallShape, VoxelPosition{.x = 3}),
        WorldgenError);
    EXPECT_THROW(
        (void)cellOf(ChunkShape{.width = 0}, VoxelPosition{}), WorldgenError);
}

TEST(ChunkShapeTest, CubeAt_UndoesCellOf)
{
    for (std::size_t cell = 0; cell < getCubeCount(kSmallShape); ++cell)
    {
        EXPECT_EQ(cellOf(kSmallShape, cubeAt(kSmallShape, cell)), cell);
    }
}

TEST(ChunkShapeTest, CubeAt_TurnsAwayACellTheChunkDoesNotHold)
{
    EXPECT_THROW(
        (void)cubeAt(kSmallShape, getCubeCount(kSmallShape)),
        WorldgenError);
}

TEST(ChunkShapeTest, Within_TurnsAwayACubeOutsideTheShape)
{
    EXPECT_TRUE(isWithin(kSmallShape, VoxelPosition{.x = 2, .y = 4, .z = 3}));
    EXPECT_FALSE(isWithin(kSmallShape, VoxelPosition{.x = -1}));
    EXPECT_FALSE(isWithin(kSmallShape, VoxelPosition{.y = -1}));
    EXPECT_FALSE(isWithin(kSmallShape, VoxelPosition{.z = -1}));
    EXPECT_FALSE(isWithin(kSmallShape, VoxelPosition{.x = 3}));
    EXPECT_FALSE(isWithin(kSmallShape, VoxelPosition{.y = 5}));
    EXPECT_FALSE(isWithin(kSmallShape, VoxelPosition{.z = 4}));
}

TEST(ChunkShapeTest, ChunkBox_CoversEveryVoxelOfEveryCube)
{
    constexpr VoxelPosition originPointPosition{.x = 2, .y = -3, .z = 5};
    const auto box = getChunkBox(kSmallShape, originPointPosition);

    for (std::size_t cell = 0; cell < getCubeCount(kSmallShape); ++cell)
    {
        const auto cube = cubeAt(kSmallShape, cell);
        const VoxelPosition cornerPosition{
            .x = (originPointPosition.x + cube.x) * antwika::voxel::kCubeSide,
            .y = (originPointPosition.y + cube.y) * antwika::voxel::kCubeSide,
            .z = (originPointPosition.z + cube.z) * antwika::voxel::kCubeSide};

        for (const auto voxel : antwika::voxel::getCubeCells(cornerPosition))
        {
            EXPECT_TRUE(holds(box, voxel));
        }
    }
}

TEST(ChunkShapeTest, Holds_LeavesTheCubeBesideTheChunkAlone)
{
    const auto box = getChunkBox(kSmallShape, VoxelPosition{});

    EXPECT_TRUE(holds(box, VoxelPosition{}));
    EXPECT_FALSE(holds(box, VoxelPosition{.x = -1}));
    EXPECT_FALSE(holds(box, VoxelPosition{.y = -1}));
    EXPECT_FALSE(holds(box, VoxelPosition{.z = -1}));
    EXPECT_FALSE(holds(box, VoxelPosition{.x = kSmallShape.width * 2}));
    EXPECT_FALSE(holds(box, VoxelPosition{.y = kSmallShape.height * 2}));
    EXPECT_FALSE(holds(box, VoxelPosition{.z = kSmallShape.depth * 2}));
}

TEST(ChunkShapeTest, ChunkBox_TurnsAwayAShapeWithNoSideToIt)
{
    EXPECT_THROW(
        (void)getChunkBox(ChunkShape{.height = 0},
            VoxelPosition{}), WorldgenError);
}
