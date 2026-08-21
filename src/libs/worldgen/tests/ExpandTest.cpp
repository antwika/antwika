#include <gtest/gtest.h>

#include <algorithm>
#include <vector>

#include <antwika/voxel/VoxelCell.hpp>
#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/worldgen/Expand.hpp>
#include <antwika/worldgen/WorldgenError.hpp>

using antwika::voxel::Facing;
using antwika::voxel::kCubeSide;
using antwika::voxel::kCubeVoxels;
using antwika::voxel::Kind;
using antwika::voxel::VoxelCell;
using antwika::worldgen::chunkVoxels;
using antwika::worldgen::WorldgenError;

TEST(ExpandTest, ChunkVoxels_LaysEightVoxelsForASolidCube)
{
    const auto chunkCells =
        chunkVoxels({VoxelCell{.x = 1, .y = 2, .z = 3, .kind = Kind::Normal}});

    ASSERT_EQ(chunkCells.size(), kCubeVoxels);

    for (const VoxelCell voxel : chunkCells)
    {
        EXPECT_GE(voxel.x, 1 * kCubeSide);
        EXPECT_LT(voxel.x, (1 * kCubeSide) + kCubeSide);
        EXPECT_GE(voxel.y, 2 * kCubeSide);
        EXPECT_LT(voxel.y, (2 * kCubeSide) + kCubeSide);
        EXPECT_GE(voxel.z, 3 * kCubeSide);
        EXPECT_LT(voxel.z, (3 * kCubeSide) + kCubeSide);
        EXPECT_EQ(voxel.kind, Kind::Normal);
    }
}

TEST(ExpandTest, ChunkVoxels_StepsARampTheWayItFaces)
{
    const auto east = chunkVoxels(
        {VoxelCell{.kind = Kind::Ramp, .facing = Facing::East}});
    const auto west = chunkVoxels(
        {VoxelCell{.kind = Kind::Ramp, .facing = Facing::West}});

    EXPECT_LT(east.size(), kCubeVoxels);
    EXPECT_EQ(east.size(), west.size());
    EXPECT_NE(east, west);
}

TEST(ExpandTest, ChunkVoxels_KeepsTheFacingOnlyOnARamp)
{
    const auto ramp = chunkVoxels(
        {VoxelCell{.kind = Kind::Ramp, .facing = Facing::North}});

    for (const VoxelCell voxel : ramp)
    {
        EXPECT_EQ(voxel.facing, Facing::North);
    }

    const auto ladder = chunkVoxels(
        {VoxelCell{.kind = Kind::Ladder, .facing = Facing::North}});

    for (const VoxelCell voxel : ladder)
    {
        EXPECT_EQ(voxel.facing, Facing::Any);
    }
}

TEST(ExpandTest, ChunkVoxels_TurnsAwayARampWithNoWayAbout)
{
    EXPECT_THROW(
        (void)chunkVoxels({VoxelCell{.kind = Kind::Ramp}}), WorldgenError);
}

TEST(ExpandTest, ChunkVoxels_LaysNothingForNoCubeAtAll)
{
    EXPECT_TRUE(chunkVoxels({}).empty());
}

TEST(ExpandTest, ChunkVoxels_LaysEveryCubeBesideTheLastRatherThanOverIt)
{
    const auto chunkCells = chunkVoxels(
        {VoxelCell{.kind = Kind::Normal},
         VoxelCell{.x = 1, .kind = Kind::Water}});

    ASSERT_EQ(chunkCells.size(), kCubeVoxels * 2);

    const auto watery = std::ranges::count_if(
        chunkCells,
        [](const VoxelCell voxel) { return voxel.kind == Kind::Water; });

    EXPECT_EQ(watery, static_cast<std::ptrdiff_t>(kCubeVoxels));
}
