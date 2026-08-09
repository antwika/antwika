#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <vector>

#include <antwika/pathfinding/AStar.hpp>
#include <antwika/pathfinding/Cost.hpp>
#include <antwika/pathfinding/NodeId.hpp>
#include <antwika/pathfinding/PathfindingError.hpp>
#include <antwika/pathfinding/SearchResult.hpp>
#include <antwika/pathfinding/fakes/FakeGraph.hpp>

namespace
{

    using antwika::pathfinding::Cost;
    using antwika::pathfinding::findPath;
    using antwika::pathfinding::nodeId;
    using antwika::pathfinding::NodeId;
    using antwika::pathfinding::PathfindingError;
    using antwika::pathfinding::SearchOutcome;
    using antwika::pathfinding::SearchResult;
    using antwika::pathfinding::fakes::FakeGraph;

    std::vector<NodeId> path(std::initializer_list<std::uint32_t> ids)
    {
        std::vector<NodeId> nodes;

        for (const std::uint32_t id : ids)
        {
            nodes.push_back(nodeId(id));
        }

        return nodes;
    }

    FakeGraph chainWithADeadEnd()
    {
        FakeGraph graph;
        graph.addEdge(nodeId(0), nodeId(1), 1);
        graph.addEdge(nodeId(1), nodeId(2), 1);
        graph.addEdge(nodeId(2), nodeId(3), 1);
        graph.addEdge(nodeId(0), nodeId(4), 1);
        graph.addEdge(nodeId(4), nodeId(5), 1);
        return graph;
    }

}

TEST(AStarHeuristicTest, FindPath_FindsTheCheapestRouteWithNoHeuristicAtAll)
{
    FakeGraph graph = chainWithADeadEnd();

    const SearchResult result = findPath(graph, nodeId(0), nodeId(3));

    EXPECT_EQ(result.outcome, SearchOutcome::PathFound);
    EXPECT_EQ(result.nodes, path({0, 1, 2, 3}));
    EXPECT_EQ(result.cost, 3);
}

TEST(AStarHeuristicTest, FindPath_ExpandsOnlyTheRouteWithAnExactHeuristic)
{
    FakeGraph graph = chainWithADeadEnd();
    graph.setHeuristic(nodeId(0), 3);
    graph.setHeuristic(nodeId(1), 2);
    graph.setHeuristic(nodeId(2), 1);
    graph.setHeuristic(nodeId(4), 1000);
    graph.setHeuristic(nodeId(5), 1000);

    const SearchResult result = findPath(graph, nodeId(0), nodeId(3));

    EXPECT_EQ(result.nodes, path({0, 1, 2, 3}));
    EXPECT_EQ(result.cost, 3);

    EXPECT_EQ(graph.expansions(), 3U);
}

TEST(AStarHeuristicTest, FindPath_ThrowsWhenTheStartEstimateIsNegative)
{
    FakeGraph graph;
    graph.addEdge(nodeId(0), nodeId(1), 1);
    graph.setHeuristic(nodeId(0), -1);

    EXPECT_THROW(
        static_cast<void>(findPath(graph, nodeId(0), nodeId(1))),
        PathfindingError);
}

TEST(AStarHeuristicTest, FindPath_ThrowsWhenANeighbourEstimateIsNegative)
{
    FakeGraph graph;
    graph.addEdge(nodeId(0), nodeId(1), 1);
    graph.addEdge(nodeId(1), nodeId(2), 1);
    graph.setHeuristic(nodeId(1), -1);

    EXPECT_THROW(
        static_cast<void>(findPath(graph, nodeId(0), nodeId(2))),
        PathfindingError);
}

TEST(AStarHeuristicTest, FindPath_ThrowsWhenACostAndItsEstimateSumPastACost)
{
    constexpr Cost kMaxCost = std::numeric_limits<Cost>::max();

    FakeGraph graph;
    graph.addEdge(nodeId(0), nodeId(1), kMaxCost);
    graph.addEdge(nodeId(1), nodeId(2), 1);
    graph.setHeuristic(nodeId(1), 1);

    EXPECT_THROW(
        static_cast<void>(findPath(graph, nodeId(0), nodeId(2))),
        PathfindingError);
}

TEST(AStarHeuristicTest, FindPath_StillReturnsARouteWhenTheEstimateOverstates)
{
    FakeGraph graph;
    graph.addEdge(nodeId(0), nodeId(1), 1);
    graph.addEdge(nodeId(1), nodeId(3), 1);
    graph.addEdge(nodeId(0), nodeId(2), 3);
    graph.addEdge(nodeId(2), nodeId(3), 3);
    graph.setHeuristic(nodeId(1), 100);

    const SearchResult result = findPath(graph, nodeId(0), nodeId(3));

    EXPECT_EQ(result.outcome, SearchOutcome::PathFound);
    EXPECT_EQ(result.nodes, path({0, 2, 3}));
    EXPECT_EQ(result.cost, 6);
}

TEST(AStarHeuristicTest, FindPath_MayOverpayWhenAnAdmissibleEstimateJumps)
{
    FakeGraph graph;
    graph.addEdge(nodeId(0), nodeId(1), 5);
    graph.addEdge(nodeId(0), nodeId(2), 1);
    graph.addEdge(nodeId(2), nodeId(1), 1);
    graph.addEdge(nodeId(1), nodeId(3), 10);
    graph.setHeuristic(nodeId(2), 5);

    const SearchResult result = findPath(graph, nodeId(0), nodeId(3));

    EXPECT_EQ(result.outcome, SearchOutcome::PathFound);
    EXPECT_EQ(result.nodes, path({0, 1, 3}));
    EXPECT_EQ(result.cost, 15);
}
