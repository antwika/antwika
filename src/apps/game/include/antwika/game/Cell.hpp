#pragma once

#include <compare>
#include <cstdint>

namespace antwika::game
{

    /**
     * @brief A position on the grid, in whole cells.
     *
     * Signed, because the grid is unbounded in principle and a camera
     * panned past the origin puts cells at negative coordinates.
     * GridExtent is what bounds the part of it anything may reach.
     *
     * Ordered, so it can key a std::map. The order is lexicographic on
     * (x, y) and carries no meaning beyond being the same every run --
     * which is the point, since a container keyed by this decides what
     * order things get drawn in.
     */
    struct Cell
    {
        std::int32_t x = 0;
        std::int32_t y = 0;

        /**
         * @brief Compare two cells.
         * @param other The cell to compare against.
         * @return True when both coordinates match.
         */
        [[nodiscard]] bool operator==(const Cell &other) const = default;

        /**
         * @brief Order two cells, by x and then y.
         * @param other The cell to compare against.
         * @return The ordering between them.
         */
        [[nodiscard]] std::strong_ordering operator<=>(
            const Cell &other) const = default;
    };

} // namespace antwika::game
