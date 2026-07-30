#pragma once

#include "antwika/pathfinding/Cost.hpp"
#include "antwika/pathfinding/NodeId.hpp"

namespace antwika::pathfinding
{

    /**
     * @brief One outgoing edge: where it lands and what it costs.
     */
    struct Neighbour
    {
        NodeId node;
        Cost cost;
    };

} // namespace antwika::pathfinding
