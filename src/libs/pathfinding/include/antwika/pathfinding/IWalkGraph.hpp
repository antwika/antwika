#pragma once

#include <vector>

#include "antwika/pathfinding/GridPos.hpp"

namespace antwika::pathfinding
{

    class IWalkGraph
    {
    public:
        virtual ~IWalkGraph() = default;

        [[nodiscard]] virtual std::vector<GridPos> neighbors(
            GridPos fromPos) const = 0;
    };

}
