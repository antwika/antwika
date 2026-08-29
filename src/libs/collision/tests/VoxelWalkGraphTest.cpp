#include <gtest/gtest.h>

#include <antwika/collision/VoxelWalkGraph.hpp>

using antwika::voxel::VoxelCell;
using antwika::voxel::Voxels;
using antwika::voxel::voxelsOf;
using antwika::collision::VoxelWalkGraph;
using antwika::pathfinding::GridPos;

namespace
{

    [[nodiscard]] Voxels getFloorOver(const std::int32_t reach)
    {
        Voxels voxels;

        for (auto x = -reach; x <= reach; ++x)
        {
            for (auto z = -reach; z <= reach; ++z)
            {
                voxels.merge(voxelsOf({VoxelCell{.position = {.x = x, .y = 0,
                    .z = z}}}));
            }
        }

        return voxels;
    }

}

TEST(VoxelWalkGraphTest, GetNeighbors_StepsToTheFourColumnsAroundASquare)
{
    const auto filledVoxels = getFloorOver(1);
    const VoxelWalkGraph walkGraph(filledVoxels);
    const auto neighborPositions = walkGraph.getNeighbors(
        GridPos{.x = 0, .y = 0, .z = 0});

    ASSERT_EQ(neighborPositions.size(), 4U);
    EXPECT_EQ(neighborPositions.at(0), (GridPos{.x = 1, .y = 0, .z = 0}));
    EXPECT_EQ(neighborPositions.at(1), (GridPos{.x = -1, .y = 0, .z = 0}));
    EXPECT_EQ(neighborPositions.at(2), (GridPos{.x = 0, .y = 0, .z = 1}));
    EXPECT_EQ(neighborPositions.at(3), (GridPos{.x = 0, .y = 0, .z = -1}));
}

TEST(VoxelWalkGraphTest, GetNeighbors_TakesAStepUpAndLeavesAWallAndAHoleOut)
{
    auto filledVoxels = getFloorOver(1);

    filledVoxels.merge(voxelsOf({VoxelCell{.position = {.x = 1, .y = 1,
        .z = 0}}}));
    filledVoxels.merge(voxelsOf({VoxelCell{.position = {.x = -1, .y = 1,
        .z = 0}}}));
    filledVoxels.merge(voxelsOf({VoxelCell{.position = {.x = -1, .y = 2,
        .z = 0}}}));
    filledVoxels.erase(antwika::voxel::VoxelPosition{.x = 0, .y = 0, .z = 1});

    const VoxelWalkGraph walkGraph(filledVoxels);
    const auto neighborPositions = walkGraph.getNeighbors(
        GridPos{.x = 0, .y = 0, .z = 0});

    ASSERT_EQ(neighborPositions.size(), 2U);
    EXPECT_EQ(neighborPositions.at(0), (GridPos{.x = 1, .y = 1, .z = 0}));
    EXPECT_EQ(neighborPositions.at(1), (GridPos{.x = 0, .y = 0, .z = -1}));
}
