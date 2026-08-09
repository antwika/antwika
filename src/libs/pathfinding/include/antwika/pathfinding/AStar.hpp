#pragma once

#include "antwika/pathfinding/IGraph.hpp"
#include "antwika/pathfinding/NodeId.hpp"
#include "antwika/pathfinding/SearchResult.hpp"

namespace antwika::pathfinding
{

    [[nodiscard]] SearchResult findPath(
        const IGraph &graph, NodeId start, NodeId goal);

}
