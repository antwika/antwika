#include <gtest/gtest.h>

#include <vector>

#include <antwika/pathfinding/AStar.hpp>
#include <antwika/pathfinding/Neighbour.hpp>
#include <antwika/pathfinding/NodeId.hpp>
#include <antwika/pathfinding/SearchResult.hpp>

#include "antwika/game/Cell.hpp"
#include "antwika/game/Direction.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/RoadGraph.hpp"

using antwika::game::Cell;
using antwika::game::cellFor;
using antwika::game::Direction;
using antwika::game::headingTo;
using antwika::game::nodeFor;
using antwika::game::PathIndex;
using antwika::game::RoadGraph;
using antwika::pathfinding::findPath;
using antwika::pathfinding::Neighbour;
using antwika::pathfinding::NodeId;
using antwika::pathfinding::rawValue;
using antwika::pathfinding::SearchOutcome;

namespace
{
    // The cells a route ran through, which is what a caller wants
    // rather than the numbers it was searched with.
    [[nodiscard]] std::vector<Cell> cellsOf(
        const std::vector<NodeId> &nodes)
    {
        std::vector<Cell> cells;
        cells.reserve(nodes.size());

        for (const auto node : nodes)
        {
            cells.push_back(cellFor(node));
        }

        return cells;
    }
} // namespace

TEST(RoadGraphTest, NodeFor_RoundTripsEveryCell)
{
    const std::vector<Cell> cells{
        {.x = 0, .y = 0},
        {.x = 3, .y = 7},
        {.x = -1, .y = -1},
        {.x = -32768, .y = 32767}};

    for (const auto cell : cells)
    {
        EXPECT_EQ(cellFor(nodeFor(cell)), cell);
    }
}

TEST(RoadGraphTest, NodeFor_IsAFunctionOfTheCellAlone)
{
    // Two calls, nothing between them, and no index consulted.
    EXPECT_EQ(nodeFor(Cell{.x = 4, .y = 9}), nodeFor(Cell{.x = 4, .y = 9}));
    EXPECT_NE(nodeFor(Cell{.x = 4, .y = 9}), nodeFor(Cell{.x = 9, .y = 4}));
}

TEST(RoadGraphTest, NodeFor_OrdersAsCellDoes)
{
    const Cell lower{.x = 1, .y = 5};
    const Cell higher{.x = 2, .y = 0};

    EXPECT_LT(lower, higher);
    EXPECT_LT(rawValue(nodeFor(lower)), rawValue(nodeFor(higher)));

    const Cell sameColumn{.x = 1, .y = 6};

    EXPECT_LT(lower, sameColumn);
    EXPECT_LT(rawValue(nodeFor(lower)), rawValue(nodeFor(sameColumn)));
}

TEST(RoadGraphTest, HeadingTo_NamesEachOfTheFourSteps)
{
    const Cell here{.x = 4, .y = 4};

    EXPECT_EQ(headingTo(here, Cell{.x = 4, .y = 3}), Direction::North);
    EXPECT_EQ(headingTo(here, Cell{.x = 4, .y = 5}), Direction::South);
    EXPECT_EQ(headingTo(here, Cell{.x = 5, .y = 4}), Direction::East);
    EXPECT_EQ(headingTo(here, Cell{.x = 3, .y = 4}), Direction::West);
}

TEST(RoadGraphTest, Neighbours_ReportsOnlyCellsWithRoads)
{
    PathIndex paths;
    (void)paths.insert(Cell{.x = 1, .y = 1});
    (void)paths.insert(Cell{.x = 1, .y = 0});
    (void)paths.insert(Cell{.x = 2, .y = 1});

    const RoadGraph roads(paths);
    std::vector<Neighbour> out;
    roads.neighbours(nodeFor(Cell{.x = 1, .y = 1}), out);

    ASSERT_EQ(out.size(), 2U);
    EXPECT_EQ(cellFor(out[0].node), (Cell{.x = 1, .y = 0}));
    EXPECT_EQ(out[0].cost, 1);
    EXPECT_EQ(cellFor(out[1].node), (Cell{.x = 2, .y = 1}));
    EXPECT_EQ(out[1].cost, 1);
}

TEST(RoadGraphTest, Neighbours_ReportsNoneOnAnIsolatedCell)
{
    PathIndex paths;
    (void)paths.insert(Cell{.x = 0, .y = 0});

    const RoadGraph roads(paths);
    std::vector<Neighbour> out;
    roads.neighbours(nodeFor(Cell{.x = 0, .y = 0}), out);

    EXPECT_TRUE(out.empty());
}

TEST(RoadGraphTest, Heuristic_IsTheManhattanDistance)
{
    const PathIndex paths;
    const RoadGraph roads(paths);

    EXPECT_EQ(
        roads.heuristic(
            nodeFor(Cell{.x = 1, .y = 2}), nodeFor(Cell{.x = 4, .y = 6})),
        7);
    EXPECT_EQ(
        roads.heuristic(
            nodeFor(Cell{.x = 4, .y = 6}), nodeFor(Cell{.x = 1, .y = 2})),
        7);
    EXPECT_EQ(
        roads.heuristic(
            nodeFor(Cell{.x = 3, .y = 3}), nodeFor(Cell{.x = 3, .y = 3})),
        0);
}

TEST(RoadGraphTest, FindPath_WalksTheRoadsAndNothingElse)
{
    // An L: (0,0) -> (0,1) -> (1,1), with (1,0) left as bare ground.
    PathIndex paths;
    (void)paths.insert(Cell{.x = 0, .y = 0});
    (void)paths.insert(Cell{.x = 0, .y = 1});
    (void)paths.insert(Cell{.x = 1, .y = 1});

    const RoadGraph roads(paths);
    const auto route = findPath(
        roads, nodeFor(Cell{.x = 1, .y = 1}), nodeFor(Cell{.x = 0, .y = 0}));

    ASSERT_EQ(route.outcome, SearchOutcome::PathFound);
    EXPECT_EQ(route.cost, 2);
    EXPECT_EQ(
        cellsOf(route.nodes),
        (std::vector<Cell>{
            {.x = 1, .y = 1}, {.x = 0, .y = 1}, {.x = 0, .y = 0}}));
}

TEST(RoadGraphTest, FindPath_ReportsNoPathAcrossAGap)
{
    PathIndex paths;
    (void)paths.insert(Cell{.x = 0, .y = 0});
    (void)paths.insert(Cell{.x = 2, .y = 0});

    const RoadGraph roads(paths);
    const auto route = findPath(
        roads, nodeFor(Cell{.x = 2, .y = 0}), nodeFor(Cell{.x = 0, .y = 0}));

    EXPECT_EQ(route.outcome, SearchOutcome::NoPath);
}

TEST(RoadGraphTest, FindPath_BreaksATieTowardsTheLowestCell)
{
    // A 2x2 block: two routes from (1,1) to (0,0), both two steps.
    PathIndex paths;
    (void)paths.insert(Cell{.x = 0, .y = 0});
    (void)paths.insert(Cell{.x = 1, .y = 0});
    (void)paths.insert(Cell{.x = 0, .y = 1});
    (void)paths.insert(Cell{.x = 1, .y = 1});

    const RoadGraph roads(paths);
    const auto route = findPath(
        roads, nodeFor(Cell{.x = 1, .y = 1}), nodeFor(Cell{.x = 0, .y = 0}));

    ASSERT_EQ(route.outcome, SearchOutcome::PathFound);
    EXPECT_EQ(route.cost, 2);

    // Both are two steps, so the tie-break decides, and it is total:
    // the same route comes back from a graph built the same way.
    const auto again = findPath(
        roads, nodeFor(Cell{.x = 1, .y = 1}), nodeFor(Cell{.x = 0, .y = 0}));

    EXPECT_EQ(route.nodes, again.nodes);
}
