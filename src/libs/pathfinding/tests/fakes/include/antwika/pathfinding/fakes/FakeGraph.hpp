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

    class FakeGraph final : public IGraph
    {
    public:
        void addEdge(NodeId from, NodeId to, Cost cost)
        {
            edges[from].push_back(Neighbour{.node = to, .cost = cost});
        }

        void link(NodeId first, NodeId second, Cost cost)
        {
            addEdge(first, second, cost);
            addEdge(second, first, cost);
        }

        void setHeuristic(NodeId node, Cost value)
        {
            heuristics[node] = value;
        }

        [[nodiscard]] std::size_t expansions() const
        {
            return expanded;
        }

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

}
