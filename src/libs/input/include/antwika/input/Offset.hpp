#pragma once

#include <cstdint>

namespace antwika::input
{

    /**
     * @brief A relative amount, in whatever units the caller asked for.
     *
     * Deliberately not a Position, though the fields match. A position
     * says where something is and a caller may not add two of them
     * together; an offset says how far something moved and adding them is
     * the whole point. Folding a tick's worth of pointer movement means
     * summing offsets, and a type that also named absolute locations
     * would make that summation look like a mistake.
     *
     * Used for both the pointer's movement and the scroll wheel's notches,
     * where x is the horizontal axis and y the vertical one.
     */
    struct Offset
    {
        std::int32_t x = 0;
        std::int32_t y = 0;

        /**
         * @brief Compare two offsets.
         * @param other The offset to compare against.
         * @return True when both axes match.
         */
        [[nodiscard]] bool operator==(const Offset &other) const = default;
    };

} // namespace antwika::input
