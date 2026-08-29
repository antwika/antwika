#include <gtest/gtest.h>

#include <vector>

#include <antwika/rules/MarkerCubes.hpp>
#include <antwika/voxel/VoxelCube.hpp>

namespace
{

    using antwika::rules::getMarkerCubeContaining;
    using antwika::voxel::VoxelPosition;

    TEST(MarkerCubesTest, MarkerCubeContaining_AnswersByTheWholeCube)
    {
        const std::vector<VoxelPosition> positions{
            VoxelPosition{.x = 3, .y = 1, .z = 3}};

        for (std::int32_t x = 2; x <= 3; ++x)
        {
            for (std::int32_t y = 0; y <= 1; ++y)
            {
                EXPECT_TRUE(
                    getMarkerCubeContaining(
                        positions,
                        VoxelPosition{.x = x, .y = y, .z = 2})
                        .has_value());
            }
        }

        EXPECT_EQ(
            getMarkerCubeContaining(
                positions, VoxelPosition{.x = 2, .y = 0, .z = 2}),
            antwika::voxel::cubeCornerOf(positions.front()));
        EXPECT_FALSE(
            getMarkerCubeContaining(
                positions, VoxelPosition{.x = 4, .y = 0, .z = 2})
                .has_value());
        EXPECT_FALSE(
            getMarkerCubeContaining(
                positions, VoxelPosition{.x = 2, .y = 2, .z = 2})
                .has_value());
    }

}
