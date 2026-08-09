#pragma once

#include <vector>

#include "antwika/pathfinding/Cost.hpp"
#include "antwika/pathfinding/Neighbour.hpp"
#include "antwika/pathfinding/NodeId.hpp"

namespace antwika::pathfinding
{

    class IGraph
    {
    public:
        virtual ~IGraph() = default;

        virtual void neighbours(
            NodeId from, std::vector<Neighbour> &out) const = 0;

        [[nodiscard]] virtual Cost heuristic(
            NodeId from, NodeId goal) const = 0;
    };

}
