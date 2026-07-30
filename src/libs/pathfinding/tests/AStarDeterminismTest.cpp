#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include <antwika/pathfinding/AStar.hpp>
#include <antwika/pathfinding/NodeId.hpp>
#include <antwika/pathfinding/SearchResult.hpp>
#include <antwika/pathfinding/fakes/FakeGraph.hpp>

namespace
{

    using antwika::pathfinding::findPath;
    using antwika::pathfinding::nodeId;
    using antwika::pathfinding::NodeId;
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

} // namespace

TEST(AStarDeterminismTest, FindPath_BreaksAnEqualCostTieOnTheLowerNodeId)
{
    FakeGraph graph;
    graph.addEdge(nodeId(0), nodeId(1), 1);
    graph.addEdge(nodeId(0), nodeId(2), 1);
    graph.addEdge(nodeId(1), nodeId(3), 1);
    graph.addEdge(nodeId(2), nodeId(3), 1);

    const SearchResult result = findPath(graph, nodeId(0), nodeId(3));

    EXPECT_EQ(result.outcome, SearchOutcome::PathFound);
    EXPECT_EQ(result.nodes, path({0, 1, 3}));
    EXPECT_EQ(result.cost, 2);
}

TEST(AStarDeterminismTest, FindPath_IgnoresTheOrderTheEdgesWereDeclaredIn)
{
    FakeGraph reversed;
    reversed.addEdge(nodeId(2), nodeId(3), 1);
    reversed.addEdge(nodeId(1), nodeId(3), 1);
    reversed.addEdge(nodeId(0), nodeId(2), 1);
    reversed.addEdge(nodeId(0), nodeId(1), 1);

    const SearchResult result = findPath(reversed, nodeId(0), nodeId(3));

    EXPECT_EQ(result.nodes, path({0, 1, 3}));
}

TEST(AStarDeterminismTest, FindPath_FollowsTheIdsWhenTheTiedNodesAreRenumbered)
{
    FakeGraph graph;
    graph.addEdge(nodeId(0), nodeId(5), 1);
    graph.addEdge(nodeId(0), nodeId(4), 1);
    graph.addEdge(nodeId(5), nodeId(9), 1);
    graph.addEdge(nodeId(4), nodeId(9), 1);

    const SearchResult result = findPath(graph, nodeId(0), nodeId(9));

    EXPECT_EQ(result.nodes, path({0, 4, 9}));
}

TEST(
    AStarDeterminismTest,
    FindPath_PrefersTheSmallerRemainingEstimateBeforeTheSmallerId)
{
    FakeGraph graph;
    graph.addEdge(nodeId(0), nodeId(1), 1);
    graph.addEdge(nodeId(0), nodeId(2), 2);
    graph.addEdge(nodeId(1), nodeId(3), 2);
    graph.addEdge(nodeId(2), nodeId(3), 1);
    graph.setHeuristic(nodeId(1), 1);

    const SearchResult result = findPath(graph, nodeId(0), nodeId(3));

    // Both routes cost 3, and both leave the start estimating 3.
    // Only the smaller remaining estimate separates them.
    // It names the larger id, which id order alone would not pick.
    EXPECT_EQ(result.nodes, path({0, 2, 3}));
    EXPECT_EQ(result.cost, 3);
}

TEST(AStarDeterminismTest, FindPath_ReturnsTheSameResultOnEveryRun)
{
    FakeGraph graph;
    graph.link(nodeId(0), nodeId(1), 1);
    graph.link(nodeId(0), nodeId(2), 1);
    graph.link(nodeId(1), nodeId(3), 1);
    graph.link(nodeId(2), nodeId(3), 1);
    graph.link(nodeId(3), nodeId(4), 1);

    const SearchResult first = findPath(graph, nodeId(0), nodeId(4));

    for (int run = 0; run < 8; ++run)
    {
        const SearchResult again = findPath(graph, nodeId(0), nodeId(4));

        EXPECT_EQ(again.outcome, first.outcome);
        EXPECT_EQ(again.nodes, first.nodes);
        EXPECT_EQ(again.cost, first.cost);
    }
}
