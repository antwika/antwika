#pragma once

#include <vector>

#include "antwika/mapcheck/Finding.hpp"

namespace antwika::mapcheck
{

    struct MapReport final
    {
        std::vector<Finding> findings{};

        /**
         * @brief Reachability per cell, indexed row * columns + column.
         */
        std::vector<bool> reachable{};
    };

}
