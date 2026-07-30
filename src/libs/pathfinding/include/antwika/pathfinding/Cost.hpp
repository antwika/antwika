#pragma once

#include <cstdint>

namespace antwika::pathfinding
{

    /**
     * @brief What one edge, one path or one heuristic estimate costs.
     *
     * Deliberately an exact integer rather than a floating-point value.
     * Every tie-break in the search rests on two costs comparing equal,
     * and "equal" has to mean the same thing on every toolchain the
     * project builds for, which a rounded sum cannot promise.
     */
    using Cost = std::int64_t;

} // namespace antwika::pathfinding
