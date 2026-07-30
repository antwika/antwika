#pragma once

#include <cstddef>
#include <map>
#include <vector>

#include <antwika/pathfinding/Cost.hpp>
#include <antwika/pathfinding/IGraph.hpp>
#include <antwika/pathfinding/Neighbour.hpp>
#include <antwika/pathfinding/NodeId.hpp>

namespace antwika::pathfinding::fakes
{

    /**
     * @brief An adjacency-list IGraph with a hand-written heuristic
     * table, for exercising the search on a world that is not a grid.
     *
     * The heuristic is keyed on the node alone and ignores the goal,
     * which is enough because a test has one goal, and it lets a test
     * state an inconsistent or inadmissible estimate outright rather
     * than contriving a geometry that produces one.
     */
    class FakeGraph final : public IGraph
    {
    public:
        /**
         * @brief Declare a one-way edge.
         * @param from Where the edge starts.
         * @param to Where it lands.
         * @param cost What traversing it costs.
         */
        void addEdge(NodeId from, NodeId to, Cost cost)
        {
            edges[from].push_back(Neighbour{.node = to, .cost = cost});
        }

        /**
         * @brief Declare an edge in both directions.
         * @param first One end.
         * @param second The other end.
         * @param cost What traversing it costs, either way.
         */
        void link(NodeId first, NodeId second, Cost cost)
        {
            addEdge(first, second, cost);
            addEdge(second, first, cost);
        }

        /**
         * @brief Set what a node estimates as its distance to the goal.
         * @param node The node to give an estimate.
         * @param value The estimate; zero unless set.
         */
        void setHeuristic(NodeId node, Cost value)
        {
            heuristics[node] = value;
        }

        /**
         * @brief How many nodes the search has asked to expand.
         * @return The neighbours() call count so far.
         */
        [[nodiscard]] std::size_t expansions() const
        {
            return expanded;
        }

        /**
         * @brief Append the declared edges out of a node.
         * @param from The node being expanded.
         * @param out The vector to append to.
         */
        void neighbours(
            NodeId from, std::vector<Neighbour> &out) const override
        {
            ++expanded;

            const auto found = edges.find(from);

            if (found == edges.end())
            {
                return;
            }

            out.insert(out.end(), found->second.begin(), found->second.end());
        }

        /**
         * @brief The estimate declared for a node.
         * @param from The node being estimated.
         * @param goal Ignored.
         * @return The declared estimate, or zero.
         */
        [[nodiscard]] Cost heuristic(
            NodeId from, NodeId goal) const override
        {
            static_cast<void>(goal);

            const auto found = heuristics.find(from);

            return found == heuristics.end() ? 0 : found->second;
        }

    private:
        std::map<NodeId, std::vector<Neighbour>> edges;
        std::map<NodeId, Cost> heuristics;
        mutable std::size_t expanded = 0;
    };

} // namespace antwika::pathfinding::fakes
