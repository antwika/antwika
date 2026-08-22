#include <gtest/gtest.h>

#include <antwika/voxel/VoxelMaterial.hpp>

TEST(VoxelCellTest, Occludes_LetsWaterShowWhatStandsInIt)
{
    using antwika::voxel::occludes;
    using antwika::voxel::Kind;

    EXPECT_TRUE(occludes(Kind::Normal, Kind::Water));
    EXPECT_TRUE(occludes(Kind::Water, Kind::Water));
    EXPECT_FALSE(occludes(Kind::Water, Kind::Normal));
    EXPECT_FALSE(occludes(Kind::Ramp, Kind::Normal));
    EXPECT_FALSE(occludes(Kind::Ramp, Kind::Water));
}
