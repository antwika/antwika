#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include <antwika/pathfinding/AStar.hpp>
#include <antwika/pathfinding/GridGraph.hpp>
#include <antwika/pathfinding/NodeId.hpp>
#include <antwika/pathfinding/SearchResult.hpp>

namespace
{

    using antwika::pathfinding::findPath;
    using antwika::pathfinding::GridGraph;
    using antwika::pathfinding::nodeId;
    using antwika::pathfinding::NodeId;
    using antwika::pathfinding::SearchOutcome;
    using antwika::pathfinding::SearchResult;

    std::vector<NodeId> path(std::initializer_list<std::uint32_t> ids)
    {
        std::vector<NodeId> nodes;

        for (const std::uint32_t id : ids)
        {
            nodes.push_back(nodeId(id));
        }

        return nodes;
    }

    GridGraph gridWithWallsAt(
        std::int32_t width,
        std::int32_t height,
        std::initializer_list<std::size_t> walls)
    {
        std::vector<bool> passable(
            static_cast<std::size_t>(width) * static_cast<std::size_t>(height),
            true);

        for (const std::size_t wall : walls)
        {
            passable[wall] = false;
        }

        return GridGraph(width, height, passable);
    }

}

TEST(GridSearchTest, FindPath_RunsStraightAcrossAnUnobstructedRow)
{
    const GridGraph grid = gridWithWallsAt(5, 1, {});

    const SearchResult result = findPath(grid, nodeId(0), nodeId(4));

    EXPECT_EQ(result.outcome, SearchOutcome::PathFound);
    EXPECT_EQ(result.nodes, path({0, 1, 2, 3, 4}));
    EXPECT_EQ(result.cost, 4);
}

TEST(GridSearchTest, FindPath_ReportsTheStartAloneEvenWhenItIsImpassable)
{
    const GridGraph grid = gridWithWallsAt(3, 3, {4});

    const SearchResult result = findPath(grid, nodeId(4), nodeId(4));

    EXPECT_EQ(result.outcome, SearchOutcome::PathFound);
    EXPECT_EQ(result.nodes, path({4}));
    EXPECT_EQ(result.cost, 0);
}

TEST(GridSearchTest, FindPath_GoesTheLongWayRoundAWall)
{
    const GridGraph grid = gridWithWallsAt(3, 3, {1, 4});

    const SearchResult result = findPath(grid, nodeId(0), nodeId(2));

    EXPECT_EQ(result.outcome, SearchOutcome::PathFound);
    EXPECT_EQ(result.nodes, path({0, 3, 6, 7, 8, 5, 2}));
    EXPECT_EQ(result.cost, 6);
}

TEST(GridSearchTest, FindPath_ReportsNoPathWhenAWallCutsTheGridInTwo)
{
    const GridGraph grid = gridWithWallsAt(3, 3, {1, 4, 7});

    const SearchResult result = findPath(grid, nodeId(0), nodeId(2));

    EXPECT_EQ(result.outcome, SearchOutcome::NoPath);
    EXPECT_TRUE(result.nodes.empty());
}

TEST(GridSearchTest, FindPath_ReportsNoPathOutOfAnImpassableStart)
{
    const GridGraph grid = gridWithWallsAt(2, 2, {0});

    const SearchResult result = findPath(grid, nodeId(0), nodeId(3));

    EXPECT_EQ(result.outcome, SearchOutcome::NoPath);
}

TEST(GridSearchTest, FindPath_ReportsNoPathIntoAnImpassableGoal)
{
    const GridGraph grid = gridWithWallsAt(2, 2, {3});

    const SearchResult result = findPath(grid, nodeId(0), nodeId(3));

    EXPECT_EQ(result.outcome, SearchOutcome::NoPath);
}

TEST(GridSearchTest, FindPath_BreaksAnEqualLengthTieTowardsTheEarlierCell)
{
    const GridGraph grid = gridWithWallsAt(2, 2, {});

    const SearchResult result = findPath(grid, nodeId(0), nodeId(3));

    EXPECT_EQ(result.nodes, path({0, 1, 3}));
    EXPECT_EQ(result.cost, 2);
}
