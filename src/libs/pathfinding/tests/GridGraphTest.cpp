#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

#include <antwika/pathfinding/Cost.hpp>
#include <antwika/pathfinding/GridGraph.hpp>
#include <antwika/pathfinding/Neighbour.hpp>
#include <antwika/pathfinding/NodeId.hpp>
#include <antwika/pathfinding/PathfindingError.hpp>

namespace
{

    using antwika::pathfinding::Cost;
    using antwika::pathfinding::GridCell;
    using antwika::pathfinding::GridGraph;
    using antwika::pathfinding::Neighbour;
    using antwika::pathfinding::nodeId;
    using antwika::pathfinding::NodeId;
    using antwika::pathfinding::PathfindingError;
    using antwika::pathfinding::rawValue;

    constexpr const char *kExtentRefusal =
        "pathfinding: a grid needs a positive width and height";

    constexpr const char *kIndexRefusal =
        "pathfinding: the grid's cell count leaves the 32-bit indices its "
        "nodes are numbered in";

    constexpr const char *kCountRefusal =
        "pathfinding: passability count does not match the grid";

    constexpr Cost kOneStep = 1;

    GridGraph openGrid()
    {
        return GridGraph(3, 3, std::vector<bool>(9, true));
    }

    GridGraph wideGrid()
    {
        return GridGraph(4, 2, std::vector<bool>(8, true));
    }

    std::string refusalFor(
        std::int32_t width, std::int32_t height, std::size_t cells)
    {
        try
        {
            static_cast<void>(
                GridGraph(width, height, std::vector<bool>(cells, true)));
        }
        catch (const PathfindingError &error)
        {
            return error.what();
        }

        return {};
    }

    std::vector<Neighbour> neighboursOf(const GridGraph &grid, NodeId from)
    {
        std::vector<Neighbour> found;
        grid.neighbours(from, found);

        return found;
    }

    std::vector<NodeId> neighbourNodes(const GridGraph &grid, NodeId from)
    {
        std::vector<NodeId> nodes;

        for (const Neighbour &neighbour : neighboursOf(grid, from))
        {
            nodes.push_back(neighbour.node);
        }

        return nodes;
    }

}

TEST(GridGraphTest, Ctor_ThrowsOnAZeroWidth)
{
    EXPECT_EQ(refusalFor(0, 3, 0), kExtentRefusal);
}

TEST(GridGraphTest, Ctor_ThrowsOnANegativeWidth)
{
    EXPECT_EQ(refusalFor(-1, 3, 0), kExtentRefusal);
}

TEST(GridGraphTest, Ctor_ThrowsOnAZeroHeight)
{
    EXPECT_EQ(refusalFor(3, 0, 0), kExtentRefusal);
}

TEST(GridGraphTest, Ctor_ThrowsOnANegativeHeight)
{
    EXPECT_EQ(refusalFor(3, -1, 0), kExtentRefusal);
}

TEST(GridGraphTest, Ctor_ThrowsWhenTheCellCountLeavesInt32)
{
    constexpr std::int32_t kTwoToTheSixteenth = 65536;

    EXPECT_EQ(
        refusalFor(kTwoToTheSixteenth, kTwoToTheSixteenth, 0),
        kIndexRefusal);
}

TEST(GridGraphTest, Ctor_ReachesThePassabilityCheckAtExactlyInt32Cells)
{
    constexpr std::int32_t kMaxInt32 =
        std::numeric_limits<std::int32_t>::max();

    EXPECT_EQ(refusalFor(1, kMaxInt32, 0), kCountRefusal);
}

TEST(GridGraphTest, Ctor_ThrowsWhenThePassabilityCountIsWrong)
{
    EXPECT_EQ(refusalFor(3, 3, 8), kCountRefusal);
}

TEST(GridGraphTest, Width_ReportsTheExtentItWasBuiltWith)
{
    const GridGraph grid = wideGrid();

    EXPECT_EQ(grid.width(), 4);
}

TEST(GridGraphTest, Height_ReportsTheExtentItWasBuiltWith)
{
    const GridGraph grid = wideGrid();

    EXPECT_EQ(grid.height(), 2);
}

TEST(GridGraphTest, Contains_RejectsEveryCellOutsideTheExtent)
{
    const GridGraph grid = wideGrid();

    EXPECT_TRUE(grid.contains(GridCell{.x = 0, .y = 0}));
    EXPECT_TRUE(grid.contains(GridCell{.x = 3, .y = 1}));
    EXPECT_FALSE(grid.contains(GridCell{.x = -1, .y = 0}));
    EXPECT_FALSE(grid.contains(GridCell{.x = 4, .y = 0}));
    EXPECT_FALSE(grid.contains(GridCell{.x = 0, .y = -1}));
    EXPECT_FALSE(grid.contains(GridCell{.x = 0, .y = 2}));
}

TEST(GridGraphTest, Passable_ReportsTheFlagTheCellWasBuiltWith)
{
    const GridGraph grid(
        3,
        2,
        std::vector<bool>{true, false, true, false, true, false});

    EXPECT_TRUE(grid.passable(GridCell{.x = 0, .y = 0}));
    EXPECT_FALSE(grid.passable(GridCell{.x = 1, .y = 0}));
    EXPECT_FALSE(grid.passable(GridCell{.x = 0, .y = 1}));
    EXPECT_TRUE(grid.passable(GridCell{.x = 1, .y = 1}));
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
    const GridGraph grid = wideGrid();

    EXPECT_EQ(grid.nodeAt(GridCell{.x = 0, .y = 0}), nodeId(0));
    EXPECT_EQ(grid.nodeAt(GridCell{.x = 3, .y = 0}), nodeId(3));
    EXPECT_EQ(grid.nodeAt(GridCell{.x = 2, .y = 1}), nodeId(6));
}

TEST(GridGraphTest, NodeAt_ThrowsForACellOutsideTheGrid)
{
    const GridGraph grid = openGrid();

    EXPECT_THROW(
        static_cast<void>(grid.nodeAt(GridCell{.x = 0, .y = 3})),
        PathfindingError);
}

TEST(GridGraphTest, CellOf_MapsARowMajorIndexBackToItsCell)
{
    const GridGraph grid = wideGrid();

    const GridCell cell = grid.cellOf(nodeId(6));

    EXPECT_EQ(cell.x, 2);
    EXPECT_EQ(cell.y, 1);
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

TEST(GridGraphTest, Neighbours_ChargesOneForEveryStep)
{
    const GridGraph grid = openGrid();

    const std::vector<Neighbour> found = neighboursOf(grid, nodeId(4));

    ASSERT_EQ(found.size(), 4U);

    for (const Neighbour &neighbour : found)
    {
        EXPECT_EQ(neighbour.cost, kOneStep) << rawValue(neighbour.node);
    }
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
    const GridGraph grid = wideGrid();

    EXPECT_EQ(grid.heuristic(nodeId(0), nodeId(6)), 3);
    EXPECT_EQ(grid.heuristic(nodeId(6), nodeId(0)), 3);
    EXPECT_EQ(grid.heuristic(nodeId(4), nodeId(4)), 0);
}

TEST(GridCellTest, OperatorEquals_ComparesBothCoordinates)
{
    constexpr GridCell base{.x = 1, .y = 2};
    constexpr GridCell twin{.x = 1, .y = 2};

    EXPECT_EQ(base, twin);
    EXPECT_NE(base, (GridCell{.x = 2, .y = 2}));
    EXPECT_NE(base, (GridCell{.x = 1, .y = 3}));
}
