#include <gtest/gtest.h>

#include <antwika/voxel/VoxelPosition.hpp>

using antwika::voxel::VoxelPosition;

TEST(VoxelPositionTest, Equality_HoldsTwoPositionsOfOneSpotTheSame)
{
    EXPECT_EQ(
        (VoxelPosition{.x = 1, .y = 2, .z = 3}),
        (VoxelPosition{.x = 1, .y = 2, .z = 3}));
}

TEST(VoxelPositionTest, Ordering_TakesXBeforeYAndYBeforeZ)
{
    EXPECT_LT(
        (VoxelPosition{.x = 0, .y = 9, .z = 9}),
        (VoxelPosition{.x = 1, .y = 0, .z = 0}));
    EXPECT_LT(
        (VoxelPosition{.x = 1, .y = 0, .z = 9}),
        (VoxelPosition{.x = 1, .y = 1, .z = 0}));
    EXPECT_LT(
        (VoxelPosition{.x = 1, .y = 1, .z = 0}),
        (VoxelPosition{.x = 1, .y = 1, .z = 1}));
}

TEST(VoxelPositionTest, Ordering_CountsOnDownBelowTheGround)
{
    EXPECT_LT(
        (VoxelPosition{.y = -1}), (VoxelPosition{.y = 0}));
}
