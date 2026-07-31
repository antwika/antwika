#include <gtest/gtest.h>

#include <vector>

#include <antwika/pathfinding/AStar.hpp>
#include <antwika/pathfinding/NodeId.hpp>
#include <antwika/pathfinding/PathfindingError.hpp>
#include <antwika/pathfinding/SearchResult.hpp>
#include <antwika/pathfinding/fakes/FakeGraph.hpp>

namespace
{

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

} // namespace

TEST(AStarTest, FindPath_ReportsTheStartAloneWhenItIsAlreadyTheGoal)
{
    FakeGraph graph;
    graph.addEdge(nodeId(0), nodeId(1), 1);

    const SearchResult result = findPath(graph, nodeId(0), nodeId(0));

    EXPECT_EQ(result.outcome, SearchOutcome::PathFound);
    EXPECT_EQ(result.nodes, path({0}));
    EXPECT_EQ(result.cost, 0);
    EXPECT_EQ(graph.expansions(), 0U);
}

TEST(AStarTest, FindPath_WalksTheOnlyRouteThroughAChain)
{
    FakeGraph graph;
    graph.addEdge(nodeId(0), nodeId(1), 2);
    graph.addEdge(nodeId(1), nodeId(2), 3);
    graph.addEdge(nodeId(2), nodeId(3), 4);

    const SearchResult result = findPath(graph, nodeId(0), nodeId(3));

    EXPECT_EQ(result.outcome, SearchOutcome::PathFound);
    EXPECT_EQ(result.nodes, path({0, 1, 2, 3}));
    EXPECT_EQ(result.cost, 9);
}

TEST(AStarTest, FindPath_ReportsNoPathWhenTheGoalIsUnreachable)
{
    FakeGraph graph;
    graph.addEdge(nodeId(0), nodeId(1), 1);
    graph.addEdge(nodeId(2), nodeId(3), 1);

    const SearchResult result = findPath(graph, nodeId(0), nodeId(3));

    EXPECT_EQ(result.outcome, SearchOutcome::NoPath);
    EXPECT_TRUE(result.nodes.empty());
    EXPECT_EQ(result.cost, 0);
}

TEST(AStarTest, FindPath_ReportsNoPathOutOfANodeWithNoEdgesAtAll)
{
    FakeGraph graph;

    const SearchResult result = findPath(graph, nodeId(7), nodeId(9));

    EXPECT_EQ(result.outcome, SearchOutcome::NoPath);
}

TEST(AStarTest, FindPath_TakesTheCheaperOfTwoRoutesToTheSameNode)
{
    FakeGraph graph;
    graph.addEdge(nodeId(0), nodeId(2), 4);
    graph.addEdge(nodeId(0), nodeId(1), 1);
    graph.addEdge(nodeId(1), nodeId(3), 1);
    graph.addEdge(nodeId(2), nodeId(3), 1);

    const SearchResult result = findPath(graph, nodeId(0), nodeId(3));

    EXPECT_EQ(result.outcome, SearchOutcome::PathFound);
    EXPECT_EQ(result.nodes, path({0, 1, 3}));
    EXPECT_EQ(result.cost, 2);
}

TEST(AStarTest, FindPath_KeepsGoingPastTheStaleEntryACheaperRouteLeaves)
{
    FakeGraph graph;
    graph.link(nodeId(0), nodeId(1), 1);
    graph.addEdge(nodeId(0), nodeId(2), 5);
    graph.addEdge(nodeId(1), nodeId(2), 1);
    graph.addEdge(nodeId(2), nodeId(3), 10);

    const SearchResult result = findPath(graph, nodeId(0), nodeId(3));

    EXPECT_EQ(result.outcome, SearchOutcome::PathFound);
    EXPECT_EQ(result.nodes, path({0, 1, 2, 3}));
    EXPECT_EQ(result.cost, 12);

    // 2 is queued twice and popped twice, but expanded once.
    EXPECT_EQ(graph.expansions(), 3U);
}

TEST(AStarTest, FindPath_AcceptsAZeroCostEdge)
{
    FakeGraph graph;
    graph.addEdge(nodeId(0), nodeId(1), 0);
    graph.addEdge(nodeId(1), nodeId(2), 0);

    const SearchResult result = findPath(graph, nodeId(0), nodeId(2));

    EXPECT_EQ(result.outcome, SearchOutcome::PathFound);
    EXPECT_EQ(result.cost, 0);
}

TEST(AStarTest, FindPath_ThrowsOnANegativeEdgeCost)
{
    FakeGraph graph;
    graph.addEdge(nodeId(0), nodeId(1), -1);

    EXPECT_THROW(
        static_cast<void>(findPath(graph, nodeId(0), nodeId(1))),
        PathfindingError);
}
