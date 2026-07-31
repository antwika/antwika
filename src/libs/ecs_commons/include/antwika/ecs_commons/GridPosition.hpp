#pragma once

#include <compare>
#include <cstdint>

namespace antwika::ecs_commons
{

    /**
     * @brief Where an entity is, in whole cells.
     *
     * Signed 32-bit integers, never floating point: this is simulation
     * state, and a replay reproduces state by re-running the same
     * arithmetic, which only holds when the arithmetic is exact.
     *
     * Ordered as well as comparable, so it can key a std::map without an
     * app supplying a comparator.
     * The order is lexicographic on (x, y) and means nothing beyond being
     * the same every run -- which is the point, since a container keyed by
     * a position decides iteration order, and iteration order decides
     * state.
     */
    struct GridPosition
    {
        std::int32_t x = 0;
        std::int32_t y = 0;

        /**
         * @brief Compare two positions.
         * @param other The position to compare against.
         * @return True when both coordinates match.
         */
        [[nodiscard]] bool operator==(const GridPosition &other) const =
            default;

        /**
         * @brief Order two positions, by x and then y.
         * @param other The position to compare against.
         * @return The ordering between them.
         */
        [[nodiscard]] std::strong_ordering operator<=>(
            const GridPosition &other) const = default;
    };

} // namespace antwika::ecs_commons
