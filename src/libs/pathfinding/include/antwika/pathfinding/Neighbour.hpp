#pragma once

#include "antwika/pathfinding/Cost.hpp"
#include "antwika/pathfinding/NodeId.hpp"

namespace antwika::pathfinding
{

    struct Neighbour final
    {
        NodeId node;
        Cost cost;
    };

}
