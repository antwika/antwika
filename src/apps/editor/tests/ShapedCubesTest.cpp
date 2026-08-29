#include <gtest/gtest.h>

#include <algorithm>

#include <antwika/editor/tools/ShapedCubes.hpp>
#include <antwika/voxel/VoxelCube.hpp>

using antwika::editor::getShapedCubes;
using antwika::editor::Paint;
using antwika::voxel::kCubeSide;
using antwika::voxel::VoxelPosition;

namespace
{
    [[nodiscard]] bool holdsCube(
        const std::vector<VoxelPosition> &cubePositions,
        const VoxelPosition wantedPosition)
    {
        return std::ranges::find(cubePositions, wantedPosition)
               != cubePositions.end();
    }
}

TEST(ShapedCubesTest, ShapedCubes_FillsTheRectBetweenTwoCorners)
{
    const auto cubes = getShapedCubes(
        VoxelPosition{},
        VoxelPosition{.x = kCubeSide, .z = kCubeSide},
        Paint::Rect);

    EXPECT_EQ(cubes.size(), 4U);
    EXPECT_TRUE(holdsCube(cubes, VoxelPosition{}));
    EXPECT_TRUE(holdsCube(cubes, VoxelPosition{.x = kCubeSide}));
    EXPECT_TRUE(holdsCube(cubes, VoxelPosition{.z = kCubeSide}));
    EXPECT_TRUE(
        holdsCube(cubes, VoxelPosition{.x = kCubeSide, .z = kCubeSide}));
}

TEST(ShapedCubesTest, ShapedCubes_WalksALineFromEndToEnd)
{
    const auto cubes = getShapedCubes(
        VoxelPosition{},
        VoxelPosition{.x = kCubeSide * 3},
        Paint::Line);

    ASSERT_EQ(cubes.size(), 4U);
    EXPECT_EQ(cubes.front(), VoxelPosition{});
    EXPECT_EQ(cubes.back(), (VoxelPosition{.x = kCubeSide * 3}));
}

TEST(ShapedCubesTest, ShapedCubes_KeepsTheLevelItStartedFrom)
{
    const auto cubes = getShapedCubes(
        VoxelPosition{.y = kCubeSide * 2},
        VoxelPosition{.x = kCubeSide * 2, .y = 0, .z = kCubeSide * 2},
        Paint::Rect);

    ASSERT_FALSE(cubes.empty());

    for (const auto cube : cubes)
    {
        EXPECT_EQ(cube.y, kCubeSide * 2);
    }
}

TEST(ShapedCubesTest, ShapedCubes_LaysOneCubeWhereBothEndsMeet)
{
    const auto cubes =
        getShapedCubes(VoxelPosition{}, VoxelPosition{}, Paint::Line);

    ASSERT_EQ(cubes.size(), 1U);
    EXPECT_EQ(cubes.front(), VoxelPosition{});
}

TEST(ShapedCubesTest, ShapedCubes_SnapsAPlaceWithinACubeToItsCorner)
{
    const auto cubes = getShapedCubes(
        VoxelPosition{.x = 1}, VoxelPosition{.x = 1}, Paint::Rect);

    ASSERT_EQ(cubes.size(), 1U);
    EXPECT_EQ(cubes.front(), antwika::voxel::cubeCornerOf(
        VoxelPosition{.x = 1}));
}
