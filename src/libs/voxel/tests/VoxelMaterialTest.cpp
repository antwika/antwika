#include <gtest/gtest.h>

#include <compare>

#include <antwika/voxel/VoxelMaterial.hpp>

TEST(VoxelMaterialTest, Occludes_LetsWaterShowWhatStandsInIt)
{
    using antwika::voxel::occludes;
    using antwika::voxel::Kind;

    EXPECT_TRUE(occludes(Kind::Normal, Kind::Water));
    EXPECT_TRUE(occludes(Kind::Water, Kind::Water));
    EXPECT_FALSE(occludes(Kind::Water, Kind::Normal));
    EXPECT_FALSE(occludes(Kind::Ramp, Kind::Normal));
    EXPECT_FALSE(occludes(Kind::Ramp, Kind::Water));
}

TEST(VoxelMaterialTest, Ordering_SortsByKindAndThenByTheWayItFaces)
{
    constexpr antwika::voxel::VoxelMaterial stoneMaterial{
        .kind = antwika::voxel::Kind::Normal,
        .facing = antwika::voxel::Facing::Any};
    constexpr antwika::voxel::VoxelMaterial waterMaterial{
        .kind = antwika::voxel::Kind::Water,
        .facing = antwika::voxel::Facing::Any};
    constexpr antwika::voxel::VoxelMaterial eastMaterial{
        .kind = antwika::voxel::Kind::Normal,
        .facing = antwika::voxel::Facing::East};

    EXPECT_TRUE(stoneMaterial < waterMaterial);
    EXPECT_TRUE(stoneMaterial < eastMaterial);
    EXPECT_FALSE(waterMaterial < stoneMaterial);
    EXPECT_TRUE(
        (stoneMaterial <=> stoneMaterial) == std::strong_ordering::equal);
}
