#include <gtest/gtest.h>

#include <antwika/voxel/VoxelCell.hpp>

using antwika::voxel::Facing;
using antwika::voxel::Kind;
using antwika::voxel::VoxelCell;

TEST(VoxelCellTest, Equality_TellsTwoKindsOfOnePlaceApart)
{
    EXPECT_NE(
        (VoxelCell{.material = {.kind = Kind::Normal}}),
        (VoxelCell{.material = {.kind = Kind::Ramp}}));
}

TEST(VoxelCellTest, Equality_TellsTwoFacingsOfOneRampApart)
{
    EXPECT_NE(
        (VoxelCell{
            .material = {.kind = Kind::Ramp, .facing = Facing::East}}),
        (VoxelCell{
            .material = {.kind = Kind::Ramp, .facing = Facing::West}}));
}

TEST(VoxelCellTest, Equality_HoldsOnePlaceOfOneMaterialTheSame)
{
    EXPECT_EQ(
        (VoxelCell{
            .position = {.x = 1, .y = 2, .z = 3},
            .material = {.kind = Kind::Water}}),
        (VoxelCell{
            .position = {.x = 1, .y = 2, .z = 3},
            .material = {.kind = Kind::Water}}));
}

TEST(VoxelCellTest, Ordering_TakesThePlaceBeforeTheMaterial)
{
    EXPECT_LT(
        (VoxelCell{
            .position = {.x = 0},
            .material = {.kind = Kind::Water}}),
        (VoxelCell{
            .position = {.x = 1},
            .material = {.kind = Kind::Normal}}));
}
