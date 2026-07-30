#pragma once

#include <cstdint>

namespace antwika::input
{

    /**
     * @brief Where a pointer was, in the backend's own surface
     * coordinates, in pixels.
     *
     * Not a window coordinate: this library does not depend on
     * antwika::gfx and cannot name a window, so what the origin refers to
     * is whatever the selected backend reports it against. An application
     * that draws through antwika::gfx decides how to relate the two.
     *
     * Signed because a pointer can legitimately be reported outside the
     * surface, e.g. while a drag continues past its edge.
     *
     * Field-for-field the same as antwika::gfx::Point, deliberately: the
     * duplication is the price of the two libraries staying independent of
     * each other.
     */
    struct Position
    {
        std::int32_t x = 0;
        std::int32_t y = 0;

        /**
         * @brief Compare two positions.
         * @param other The position to compare against.
         * @return True when both coordinates match.
         */
        [[nodiscard]] bool operator==(const Position &other) const = default;
    };

} // namespace antwika::input
