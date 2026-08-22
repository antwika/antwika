#include <gtest/gtest.h>

#include <antwika/voxel/VoxelMaterial.hpp>
#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/voxel/Voxels.hpp>

using antwika::voxel::Facing;
using antwika::voxel::Kind;
using antwika::voxel::VoxelMaterial;
using antwika::voxel::VoxelPosition;
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

TEST(VoxelsTest, Voxels_LaysItsPlacesOutLowestFirst)
{
    Voxels voxels;

    voxels[VoxelPosition{.x = 2}] = VoxelMaterial{};
    voxels[VoxelPosition{.x = 1}] = VoxelMaterial{};

    EXPECT_EQ(voxels.begin()->first, (VoxelPosition{.x = 1}));
}
