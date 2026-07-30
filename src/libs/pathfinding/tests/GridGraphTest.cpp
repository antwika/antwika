#include <gtest/gtest.h>

#include <vector>

#include <antwika/pathfinding/GridGraph.hpp>
#include <antwika/pathfinding/Neighbour.hpp>
#include <antwika/pathfinding/NodeId.hpp>
#include <antwika/pathfinding/PathfindingError.hpp>

namespace
{

    using antwika::pathfinding::GridCell;
    using antwika::pathfinding::GridGraph;
    using antwika::pathfinding::kGridStepCost;
    using antwika::pathfinding::Neighbour;
    using antwika::pathfinding::nodeId;
    using antwika::pathfinding::NodeId;
    using antwika::pathfinding::PathfindingError;

    GridGraph openGrid()
    {
        return GridGraph(3, 3, std::vector<bool>(9, true));
    }

    std::vector<NodeId> neighbourNodes(const GridGraph &grid, NodeId from)
    {
        std::vector<Neighbour> found;
        grid.neighbours(from, found);

        std::vector<NodeId> nodes;

        for (const Neighbour &neighbour : found)
        {
            EXPECT_EQ(neighbour.cost, kGridStepCost);
            nodes.push_back(neighbour.node);
        }

        return nodes;
    }

} // namespace

TEST(GridGraphTest, Constructor_ThrowsOnANonPositiveWidth)
{
    EXPECT_THROW(
        static_cast<void>(GridGraph(0, 3, std::vector<bool>(0, true))),
        PathfindingError);
}

TEST(GridGraphTest, Constructor_ThrowsOnANonPositiveHeight)
{
    EXPECT_THROW(
        static_cast<void>(GridGraph(3, -1, std::vector<bool>(0, true))),
        PathfindingError);
}

TEST(GridGraphTest, Constructor_ThrowsWhenThePassabilityCountIsWrong)
{
    EXPECT_THROW(
        static_cast<void>(GridGraph(3, 3, std::vector<bool>(8, true))),
        PathfindingError);
}

TEST(GridGraphTest, Width_ReportsTheExtentItWasBuiltWith)
{
    const GridGraph grid(4, 2, std::vector<bool>(8, true));

    EXPECT_EQ(grid.width(), 4);
    EXPECT_EQ(grid.height(), 2);
}

TEST(GridGraphTest, Contains_RejectsEveryCellOutsideTheExtent)
{
    const GridGraph grid = openGrid();

    EXPECT_TRUE(grid.contains(GridCell{.x = 0, .y = 0}));
    EXPECT_TRUE(grid.contains(GridCell{.x = 2, .y = 2}));
    EXPECT_FALSE(grid.contains(GridCell{.x = -1, .y = 0}));
    EXPECT_FALSE(grid.contains(GridCell{.x = 3, .y = 0}));
    EXPECT_FALSE(grid.contains(GridCell{.x = 0, .y = -1}));
    EXPECT_FALSE(grid.contains(GridCell{.x = 0, .y = 3}));
}

TEST(GridGraphTest, Passable_ReportsTheFlagTheCellWasBuiltWith)
{
    const GridGraph grid(
        2, 1, std::vector<bool>{true, false});

    EXPECT_TRUE(grid.passable(GridCell{.x = 0, .y = 0}));
    EXPECT_FALSE(grid.passable(GridCell{.x = 1, .y = 0}));
}

TEST(GridGraphTest, Passable_ThrowsForACellOutsideTheGrid)
{
    const GridGraph grid = openGrid();

    EXPECT_THROW(
        static_cast<void>(grid.passable(GridCell{.x = 3, .y = 0})),
        PathfindingError);
}

TEST(GridGraphTest, NodeAt_NumbersCellsInRowMajorOrder)
{
    const GridGraph grid = openGrid();

    EXPECT_EQ(grid.nodeAt(GridCell{.x = 0, .y = 0}), nodeId(0));
    EXPECT_EQ(grid.nodeAt(GridCell{.x = 2, .y = 0}), nodeId(2));
    EXPECT_EQ(grid.nodeAt(GridCell{.x = 1, .y = 2}), nodeId(7));
}

TEST(GridGraphTest, NodeAt_ThrowsForACellOutsideTheGrid)
{
    const GridGraph grid = openGrid();

    EXPECT_THROW(
        static_cast<void>(grid.nodeAt(GridCell{.x = 0, .y = 3})),
        PathfindingError);
}

TEST(GridGraphTest, CellOf_InvertsNodeAt)
{
    const GridGraph grid = openGrid();

    EXPECT_EQ(grid.cellOf(nodeId(0)), (GridCell{.x = 0, .y = 0}));
    EXPECT_EQ(grid.cellOf(nodeId(7)), (GridCell{.x = 1, .y = 2}));
    EXPECT_NE(grid.cellOf(nodeId(0)), (GridCell{.x = 1, .y = 0}));
    EXPECT_NE(grid.cellOf(nodeId(0)), (GridCell{.x = 0, .y = 1}));
}

TEST(GridGraphTest, CellOf_ThrowsForANodeOutsideTheGrid)
{
    const GridGraph grid = openGrid();

    EXPECT_THROW(
        static_cast<void>(grid.cellOf(nodeId(9))), PathfindingError);
}

TEST(GridGraphTest, Neighbours_ReportsAllFourOrthogonalCellsInRowMajorOrder)
{
    const GridGraph grid = openGrid();

    EXPECT_EQ(
        neighbourNodes(grid, nodeId(4)),
        (std::vector<NodeId>{nodeId(1), nodeId(3), nodeId(5), nodeId(7)}));
}

TEST(GridGraphTest, Neighbours_SkipsTheCellsOffEveryEdge)
{
    const GridGraph grid = openGrid();

    EXPECT_EQ(
        neighbourNodes(grid, nodeId(0)),
        (std::vector<NodeId>{nodeId(1), nodeId(3)}));
    EXPECT_EQ(
        neighbourNodes(grid, nodeId(8)),
        (std::vector<NodeId>{nodeId(5), nodeId(7)}));
}

TEST(GridGraphTest, Neighbours_SkipsAnImpassableCell)
{
    std::vector<bool> passable(9, true);
    passable[3] = false;
    const GridGraph grid(3, 3, passable);

    EXPECT_EQ(
        neighbourNodes(grid, nodeId(4)),
        (std::vector<NodeId>{nodeId(1), nodeId(5), nodeId(7)}));
}

TEST(GridGraphTest, Neighbours_ReportsNothingForAnImpassableCell)
{
    std::vector<bool> passable(9, true);
    passable[4] = false;
    const GridGraph grid(3, 3, passable);

    EXPECT_TRUE(neighbourNodes(grid, nodeId(4)).empty());
}

TEST(GridGraphTest, Heuristic_IsTheManhattanDistanceEitherWayRound)
{
    const GridGraph grid = openGrid();

    EXPECT_EQ(grid.heuristic(nodeId(0), nodeId(8)), 4);
    EXPECT_EQ(grid.heuristic(nodeId(8), nodeId(0)), 4);
    EXPECT_EQ(grid.heuristic(nodeId(4), nodeId(4)), 0);
}
