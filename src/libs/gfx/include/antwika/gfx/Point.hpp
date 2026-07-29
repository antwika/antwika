#pragma once

#include <cstdint>

namespace antwika::gfx
{

    /**
     * @brief A position in a window's drawable area, in pixels.
     *
     * Signed because a position can legitimately fall outside the
     * drawable area, e.g. a rectangle that is partly scrolled off the
     * left edge.
     */
    struct Point
    {
        std::int32_t x = 0;
        std::int32_t y = 0;
    };

} // namespace antwika::gfx
