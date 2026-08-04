#pragma once

#include "antwika/geometry/Point.hpp"
#include "antwika/geometry/Size.hpp"

namespace antwika::geometry
{

    /**
     * @brief An axis-aligned rectangle, given by its top-left corner.
     */
    struct Rect
    {
        Point origin;
        Size size;

        /**
         * @brief Compare two rectangles.
         * @param other The rectangle to compare against.
         * @return True when both origin and size match.
         */
        [[nodiscard]] bool operator==(const Rect &other) const = default;
    };

} // namespace antwika::geometry
