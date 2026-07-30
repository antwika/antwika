#pragma once

#include <vector>

#include "antwika/pathfinding/Cost.hpp"
#include "antwika/pathfinding/Neighbour.hpp"
#include "antwika/pathfinding/NodeId.hpp"

namespace antwika::pathfinding
{

    /**
     * @brief The whole of what findPath() knows about a world.
     *
     * There is no node table, no extent and no geometry here on
     * purpose: a graph is whatever answers these two questions, so a
     * grid, a road network and a room adjacency all reach the same
     * search without the library growing a cell type. GridGraph is one
     * implementation layered on top, not a privileged one.
     */
    class IGraph
    {
    public:
        virtual ~IGraph() = default;

        /**
         * @brief Append the edges leading out of a node.
         * @param from The node being expanded.
         * @param out The vector to append to; the search clears it
         * before every call, so an implementation only ever appends,
         * and reuses the caller's buffer rather than allocating.
         */
        virtual void neighbours(
            NodeId from, std::vector<Neighbour> &out) const = 0;

        /**
         * @brief Estimate what it costs to get from a node to the goal.
         * @param from The node being estimated.
         * @param goal The goal the search is heading for.
         * @return A non-negative estimate.
         * @note The search assumes the estimate is *consistent*, not
         * merely admissible: `heuristic(a, goal)` must be no greater
         * than the cost of any edge `a -> b` plus `heuristic(b, goal)`.
         * A node is closed once and never reopened, so an inconsistent
         * estimate still returns a path, and still terminates, but that
         * path may cost more than the cheapest one. Returning 0 is
         * always consistent, and turns the search into Dijkstra's.
         */
        [[nodiscard]] virtual Cost heuristic(
            NodeId from, NodeId goal) const = 0;
    };

} // namespace antwika::pathfinding
