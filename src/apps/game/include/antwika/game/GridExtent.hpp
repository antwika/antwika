#pragma once

#include <cstdint>

#include "antwika/game/Cell.hpp"

namespace antwika::game
{

    /**
     * @brief The rectangle of cells, from the origin, that anything may
     * occupy.
     *
     * Bounds what a click can reach and what gets drawn, without bounding
     * what has to be allocated: paths are held sparsely, so this costs
     * nothing per cell. Without it, a click on empty space far outside the
     * board would create a path nobody can see and nothing can reach.
     */
    struct GridExtent
    {
        std::int32_t width = 0;
        std::int32_t height = 0;

        /**
         * @brief Check whether a cell is inside the extent.
         * @param cell The cell to test.
         * @return True when both coordinates are within bounds. Always
         * false for a negative coordinate, since the extent starts at the
         * origin.
         */
        [[nodiscard]] constexpr bool contains(Cell cell) const noexcept
        {
            return cell.x >= 0 && cell.x < width && cell.y >= 0
                   && cell.y < height;
        }

        /**
         * @brief Compare two extents.
         * @param other The extent to compare against.
         * @return True when both dimensions match.
         */
        [[nodiscard]] bool operator==(
            const GridExtent &other) const = default;
    };

} // namespace antwika::game
