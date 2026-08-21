#include <gtest/gtest.h>

#include <vector>

#include <antwika/rules/Gates.hpp>
#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/tilemap/TileEdges.hpp>

#include "antwika/rules/Gates.hpp"

namespace
{

    using antwika::rules::cubeOccupied;
    using antwika::rules::adjacentDoor;
    using antwika::rules::doorwayCells;
    using antwika::rules::gateCubeContaining;
    using antwika::voxel::kCubeSide;
    using antwika::voxel::VoxelCell;

    TEST(GatesTest, GateCubeContaining_AnswersByTheWholeCube)
    {
        const std::vector<VoxelCell> cells{
            VoxelCell{.x = 3, .y = 1, .z = 3}};

        for (std::int32_t x = 2; x <= 3; ++x)
        {
            for (std::int32_t y = 0; y <= 1; ++y)
            {
                EXPECT_TRUE(
                    gateCubeContaining(
                        cells,
                        VoxelCell{.x = x, .y = y, .z = 2})
                        .has_value());
            }
        }

        EXPECT_EQ(
            gateCubeContaining(
                cells, VoxelCell{.x = 2, .y = 0, .z = 2}),
            antwika::voxel::cubeCornerOf(cells.front()));
        EXPECT_FALSE(
            gateCubeContaining(
                cells, VoxelCell{.x = 4, .y = 0, .z = 2})
                .has_value());
        EXPECT_FALSE(
            gateCubeContaining(
                cells, VoxelCell{.x = 2, .y = 2, .z = 2})
                .has_value());
    }

    TEST(GatesTest, AdjacentDoor_LooksAStepOutEachWay)
    {
        const std::vector<VoxelCell> doorCells{
            VoxelCell{.x = 4, .y = 0, .z = 2}};
        const VoxelCell middleCell{.x = 2, .y = 0, .z = 2};

        EXPECT_TRUE(adjacentDoor(doorCells, middleCell).has_value());
        EXPECT_TRUE(
            adjacentDoor(
                doorCells, VoxelCell{.x = 6, .y = 1, .z = 3})
                .has_value());
        EXPECT_TRUE(
            adjacentDoor(
                doorCells, VoxelCell{.x = 4, .y = 0, .z = 0})
                .has_value());
        EXPECT_TRUE(
            adjacentDoor(
                doorCells, VoxelCell{.x = 5, .y = 0, .z = 4})
                .has_value());
        EXPECT_FALSE(
            adjacentDoor(
                doorCells, VoxelCell{.x = 0, .y = 0, .z = 0})
                .has_value());
        EXPECT_FALSE(
            adjacentDoor(
                doorCells, VoxelCell{.x = 6, .y = 0, .z = 4})
                .has_value());
    }

    TEST(GatesTest, DoorwayCells_GathersOneDoorwayWhole)
    {
        const std::vector<VoxelCell> doorCells{
            VoxelCell{.x = 4, .y = 0, .z = 2},
            VoxelCell{.x = 4, .y = 1, .z = 2},
            VoxelCell{.x = 5, .y = 2, .z = 3},
            VoxelCell{.x = 8, .y = 0, .z = 2}};
        const auto corner =
            antwika::voxel::cubeCornerOf(doorCells.front());
        const auto column = doorwayCells(doorCells, corner);

        ASSERT_EQ(column.size(), 3U);
        EXPECT_EQ(
            doorwayCells(doorCells, VoxelCell{.x = 0, .z = 0})
                .size(),
            0U);
    }

    TEST(GatesTest, CubeOccupied_SeesAnyVoxelOfTheCube)
    {
        const std::vector<VoxelCell> voxels{
            VoxelCell{.x = 3, .y = 1, .z = 3}};

        EXPECT_TRUE(
            cubeOccupied(voxels, VoxelCell{.x = 2, .z = 2}));
        EXPECT_FALSE(
            cubeOccupied(voxels, VoxelCell{.x = 4, .z = 2}));
        EXPECT_FALSE(
            cubeOccupied(
                voxels, VoxelCell{.x = 2, .y = 2, .z = 2}));
    }

}
