#include <gtest/gtest.h>

#include <antwika/voxel/VoxelMaterial.hpp>
#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/voxel/Voxels.hpp>

using antwika::voxel::Facing;
using antwika::voxel::Kind;
using antwika::voxel::VoxelMaterial;
using antwika::voxel::VoxelPosition;
using antwika::voxel::getSortedCells;
using antwika::voxel::Voxels;

TEST(VoxelsTest, Voxels_StandsOneMaterialInEachPlace)
{
    const VoxelPosition position{.x = 1, .y = 2, .z = 3};
    Voxels voxels;

    voxels[position] = VoxelMaterial{.kind = Kind::Normal};
    voxels[position] = VoxelMaterial{.kind = Kind::Water};

    EXPECT_EQ(voxels.size(), 1U);
    EXPECT_EQ(voxels.at(position).kind, Kind::Water);
}

TEST(VoxelsTest, Voxels_TellsTwoPlacesApartThatDifferInOneStep)
{
    Voxels voxels;

    voxels[VoxelPosition{.x = 1}] = VoxelMaterial{.kind = Kind::Normal};
    voxels[VoxelPosition{.x = 2}] = VoxelMaterial{.kind = Kind::Water};

    EXPECT_EQ(voxels.size(), 2U);
}

TEST(VoxelsTest, Voxels_KeepsTheMaterialOfThePlaceItIsAskedFor)
{
    Voxels voxels;

    voxels[VoxelPosition{}] =
        VoxelMaterial{.kind = Kind::Ramp, .facing = Facing::East};

    EXPECT_EQ(
        voxels.at(VoxelPosition{}),
        (VoxelMaterial{.kind = Kind::Ramp, .facing = Facing::East}));
}

TEST(VoxelsTest, SortedCells_LaysThePlacesOutLowestFirst)
{
    Voxels voxels;

    voxels[VoxelPosition{.x = 2}] = VoxelMaterial{};
    voxels[VoxelPosition{.x = 1, .z = 1}] = VoxelMaterial{};
    voxels[VoxelPosition{.x = 1}] = VoxelMaterial{};

    const auto cells = getSortedCells(voxels);

    ASSERT_EQ(cells.size(), 3U);
    EXPECT_EQ(cells[0].position, (VoxelPosition{.x = 1}));
    EXPECT_EQ(cells[1].position, (VoxelPosition{.x = 1, .z = 1}));
    EXPECT_EQ(cells[2].position, (VoxelPosition{.x = 2}));
}

TEST(VoxelsTest, SortedCells_HandsBackNothingForAWorldWithNoCells)
{
    EXPECT_TRUE(getSortedCells(Voxels{}).empty());
}

TEST(VoxelsTest, SortedCells_CarriesTheMaterialEachPlaceHolds)
{
    Voxels voxels;

    voxels[VoxelPosition{.x = 1}] =
        VoxelMaterial{.kind = Kind::Ramp, .facing = Facing::East};

    const auto cells = getSortedCells(voxels);

    ASSERT_EQ(cells.size(), 1U);
    EXPECT_EQ(cells[0].material.kind, Kind::Ramp);
    EXPECT_EQ(cells[0].material.facing, Facing::East);
}

TEST(VoxelsTest, PositionHash_PartsPlacesThatDifferInOneStep)
{
    const antwika::voxel::VoxelPositionHash hashOf;

    EXPECT_NE(hashOf(VoxelPosition{}), hashOf(VoxelPosition{.x = 1}));
    EXPECT_NE(hashOf(VoxelPosition{}), hashOf(VoxelPosition{.y = 1}));
    EXPECT_NE(hashOf(VoxelPosition{}), hashOf(VoxelPosition{.z = 1}));
    EXPECT_NE(
        hashOf(VoxelPosition{.x = 1}), hashOf(VoxelPosition{.z = 1}));
    EXPECT_EQ(
        hashOf(VoxelPosition{.x = 3, .y = -2, .z = 7}),
        hashOf(VoxelPosition{.x = 3, .y = -2, .z = 7}));
}
