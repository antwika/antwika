#pragma once

#include "antwika/pathfinding/IGraph.hpp"
#include "antwika/pathfinding/NodeId.hpp"
#include "antwika/pathfinding/SearchResult.hpp"

namespace antwika::pathfinding
{

    /**
     * @brief Find the cheapest route from one node to another.
     *
     * The search is A* over whatever `graph` describes, and it is
     * deterministic down to which of two equally cheap paths comes
     * back: the open set is ordered by estimated total cost, then by
     * the heuristic estimate remaining, then by ascending NodeId. That
     * last key is what makes the order total, since no two entries in
     * the open set name the same node at the same cost, so nothing in
     * the result depends on the order edges were declared in, on how
     * the heap happened to arrange equal keys, or on any address.
     * Where two routes reach a node for the same cost, the one found
     * first under that order is kept -- a later tie never displaces an
     * earlier one.
     *
     * @param graph The world to search.
     * @param start Where the route begins.
     * @param goal Where the route ends.
     * @return A SearchResult that is PathFound with a start-to-goal
     * node list, or NoPath if no route exists.
     * @throws PathfindingError If the graph reports a negative edge
     * cost or a negative heuristic estimate.
     */
    [[nodiscard]] SearchResult findPath(
        const IGraph &graph, NodeId start, NodeId goal);

} // namespace antwika::pathfinding
