#pragma once

#include <vector>

#include "antwika/mapcheck/Finding.hpp"

namespace antwika::mapcheck
{

    struct CellReach final
    {
        /**
         * @brief Whether the column offers any standable walkable
         *        surface.
         *
         * Ensures: walkability is judged with the final held tag
         *          set, the same state anyReached reflects.
         */
        bool anyStandable = false;

        bool anyReached = false;

        [[nodiscard]] bool operator==(const CellReach &other) const
            = default;
    };

    struct MapReport final
    {
        std::vector<Finding> findings{};

        /**
         * @brief Reach per cell, indexed row * columns + column.
         */
        std::vector<CellReach> reachable{};
    };

}
