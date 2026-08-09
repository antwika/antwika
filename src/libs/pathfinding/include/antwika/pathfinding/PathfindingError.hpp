#pragma once

#include <stdexcept>

namespace antwika::pathfinding
{

    class PathfindingError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

}
