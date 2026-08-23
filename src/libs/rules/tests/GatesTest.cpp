#include <gtest/gtest.h>

#include <vector>

#include <antwika/rules/Gates.hpp>
#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/tilemap/TileEdges.hpp>

#include "antwika/rules/Gates.hpp"

namespace
{

    using antwika::rules::isCubeOccupied;
    using antwika::rules::getAdjacentDoor;
    using antwika::rules::getDoorwayCells;
    using antwika::rules::getGateCubeContaining;
    using antwika::voxel::kCubeSide;
    using antwika::voxel::VoxelCell;
using antwika::voxel::voxelsOf;
using antwika::voxel::VoxelPosition;

    TEST(GatesTest, GateCubeContaining_AnswersByTheWholeCube)
    {
        const std::vector<VoxelPosition> positions{
            VoxelPosition{.x = 3, .y = 1, .z = 3}};

        for (std::int32_t x = 2; x <= 3; ++x)
        {
            for (std::int32_t y = 0; y <= 1; ++y)
            {
                EXPECT_TRUE(
                    getGateCubeContaining(
                        positions,
                        VoxelPosition{.x = x, .y = y, .z = 2})
                        .has_value());
            }
        }

        EXPECT_EQ(
            getGateCubeContaining(
                positions, VoxelPosition{.x = 2, .y = 0, .z = 2}),
            antwika::voxel::cubeCornerOf(positions.front()));
        EXPECT_FALSE(
            getGateCubeContaining(
                positions, VoxelPosition{.x = 4, .y = 0, .z = 2})
                .has_value());
        EXPECT_FALSE(
            getGateCubeContaining(
                positions, VoxelPosition{.x = 2, .y = 2, .z = 2})
                .has_value());
    }

    TEST(GatesTest, AdjacentDoor_LooksAStepOutEachWay)
    {
        const std::vector<VoxelPosition> doorPositions{
            VoxelPosition{.x = 4, .y = 0, .z = 2}};
        const VoxelPosition middlePosition{.x = 2, .y = 0, .z = 2};

        EXPECT_TRUE(getAdjacentDoor(doorPositions, middlePosition).has_value());
        EXPECT_TRUE(
            getAdjacentDoor(
                doorPositions, VoxelPosition{.x = 6, .y = 1, .z = 3})
                .has_value());
        EXPECT_TRUE(
            getAdjacentDoor(
                doorPositions, VoxelPosition{.x = 4, .y = 0, .z = 0})
                .has_value());
        EXPECT_TRUE(
            getAdjacentDoor(
                doorPositions, VoxelPosition{.x = 5, .y = 0, .z = 4})
                .has_value());
        EXPECT_FALSE(
            getAdjacentDoor(
                doorPositions, VoxelPosition{.x = 0, .y = 0, .z = 0})
                .has_value());
        EXPECT_FALSE(
            getAdjacentDoor(
                doorPositions, VoxelPosition{.x = 6, .y = 0, .z = 4})
                .has_value());
    }

    TEST(GatesTest, DoorwayCells_GathersOneDoorwayWhole)
    {
        const std::vector<VoxelPosition> doorPositions{
            VoxelPosition{.x = 4, .y = 0, .z = 2},
            VoxelPosition{.x = 4, .y = 1, .z = 2},
            VoxelPosition{.x = 5, .y = 2, .z = 3},
            VoxelPosition{.x = 8, .y = 0, .z = 2}};
        const auto corner =
            antwika::voxel::cubeCornerOf(doorPositions.front());
        const auto column = getDoorwayCells(doorPositions, corner);

        ASSERT_EQ(column.size(), 3U);
        EXPECT_EQ(
            getDoorwayCells(doorPositions, VoxelPosition{.x = 0, .z = 0})
                .size(),
            0U);
    }

    TEST(GatesTest, CubeOccupied_SeesAnyVoxelOfTheCube)
    {
        const auto voxels = voxelsOf({VoxelCell{.position = {.x = 3, .y = 1,
            .z = 3}}});

        EXPECT_TRUE(
            isCubeOccupied(voxels, VoxelPosition{.x = 2, .z = 2}));
        EXPECT_FALSE(
            isCubeOccupied(voxels, VoxelPosition{.x = 4, .z = 2}));
        EXPECT_FALSE(
            isCubeOccupied(
                voxels, VoxelPosition{.x = 2, .y = 2, .z = 2}));
    }

}
