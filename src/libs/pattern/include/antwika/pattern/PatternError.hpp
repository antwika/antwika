#pragma once

#include <stdexcept>

namespace antwika::pattern
{

    /**
     * @brief Thrown when a pattern could not be described exactly.
     *
     * Every cause is arithmetic this library refuses to approximate: a
     * denominator of zero, a division by a zero-length cycle, a span
     * that ends where it began or before it, and an exact rational that
     * will not fit in the integers backing it.
     *
     * **Refusing rather than rounding is the whole point.**
     * A pattern that quietly reduced to the nearest representable
     * fraction would put a note somewhere near where the score said, and
     * nothing downstream could tell how far it had drifted.
     * That is the same call antwika::tween makes when an easing's
     * denominator will not fit, and antwika::pathfinding makes about a
     * cost that overflows.
     *
     * Deliberately a single, specific, catchable type.
     */
    class PatternError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

} // namespace antwika::pattern
