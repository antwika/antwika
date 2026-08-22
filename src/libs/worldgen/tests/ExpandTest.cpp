#include <gtest/gtest.h>

#include <algorithm>

#include <antwika/voxel/VoxelCell.hpp>
#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/voxel/Voxels.hpp>
#include <antwika/worldgen/Expand.hpp>

using antwika::voxel::Facing;
using antwika::voxel::kCubeSide;
using antwika::voxel::kCubeVoxels;
using antwika::voxel::Kind;
using antwika::voxel::VoxelCell;
using antwika::voxel::voxelsOf;
using antwika::worldgen::chunkVoxels;

TEST(ExpandTest, ChunkVoxels_LaysEightVoxelsForASolidCube)
{
    const auto chunkCells = chunkVoxels(
        voxelsOf({VoxelCell{.x = 1, .y = 2, .z = 3, .kind = Kind::Normal}}));

    ASSERT_EQ(chunkCells.size(), kCubeVoxels);

    for (const auto &[position, material] : chunkCells)
    {
        EXPECT_GE(position.x, 1 * kCubeSide);
        EXPECT_LT(position.x, (1 * kCubeSide) + kCubeSide);
        EXPECT_GE(position.y, 2 * kCubeSide);
        EXPECT_LT(position.y, (2 * kCubeSide) + kCubeSide);
        EXPECT_GE(position.z, 3 * kCubeSide);
        EXPECT_LT(position.z, (3 * kCubeSide) + kCubeSide);
        EXPECT_EQ(material.kind, Kind::Normal);
    }
}

TEST(ExpandTest, ChunkVoxels_StepsARampTheWayItFaces)
{
    const auto east = chunkVoxels(
        voxelsOf({VoxelCell{.kind = Kind::Ramp, .facing = Facing::East}}));
    const auto west = chunkVoxels(
        voxelsOf({VoxelCell{.kind = Kind::Ramp, .facing = Facing::West}}));

    EXPECT_LT(east.size(), kCubeVoxels);
    EXPECT_EQ(east.size(), west.size());
    EXPECT_NE(east, west);
}

TEST(ExpandTest, ChunkVoxels_KeepsTheFacingOnlyOnARamp)
{
    const auto ramp = chunkVoxels(
        voxelsOf({VoxelCell{.kind = Kind::Ramp, .facing = Facing::North}}));

    for (const auto &[position, material] : ramp)
    {
        EXPECT_EQ(material.facing, Facing::North);
    }

    const auto ladder = chunkVoxels(
        voxelsOf({VoxelCell{.kind = Kind::Ladder, .facing = Facing::North}}));

    for (const auto &[position, material] : ladder)
    {
        EXPECT_EQ(material.facing, Facing::Any);
    }
}

TEST(ExpandTest, ChunkVoxels_WorksOutTheWayARampToldNothingClimbs)
{
    const auto chunkCells = chunkVoxels(
        voxelsOf({VoxelCell{.kind = Kind::Ramp}}));

    EXPECT_FALSE(chunkCells.empty());
    EXPECT_LT(chunkCells.size(), kCubeVoxels);
}

TEST(ExpandTest, ChunkVoxels_ClimbsARampTowardsTheGroundBesideIt)
{
    const auto chunkCells = chunkVoxels(
        voxelsOf({VoxelCell{.kind = Kind::Ramp},
                  VoxelCell{.x = 1, .kind = Kind::Normal}}));
    const auto east = chunkVoxels(
        voxelsOf({VoxelCell{.kind = Kind::Ramp, .facing = Facing::East},
                  VoxelCell{.x = 1, .kind = Kind::Normal}}));

    EXPECT_EQ(chunkCells.size(), east.size());
}

TEST(ExpandTest, ChunkVoxels_LaysNothingForNoCubeAtAll)
{
    EXPECT_TRUE(chunkVoxels({}).empty());
}

TEST(ExpandTest, ChunkVoxels_LaysEveryCubeBesideTheLastRatherThanOverIt)
{
    const auto chunkCells = chunkVoxels(
        voxelsOf({VoxelCell{.kind = Kind::Normal},
                  VoxelCell{.x = 1, .kind = Kind::Water}}));

    ASSERT_EQ(chunkCells.size(), kCubeVoxels * 2);

    const auto watery = std::ranges::count_if(
        chunkCells,
        [](const auto &standing)
        { return standing.second.kind == Kind::Water; });

    EXPECT_EQ(watery, static_cast<std::ptrdiff_t>(kCubeVoxels));
}
