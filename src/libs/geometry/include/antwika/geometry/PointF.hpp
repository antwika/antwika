#pragma once

#include "antwika/geometry/Point.hpp"

namespace antwika::geometry
{

    struct PointF final
    {
        float x = 0.0F;
        float y = 0.0F;

        constexpr PointF() noexcept = default;

        constexpr PointF(const float x, const float y) noexcept
            : x(x), y(y)
        {
        }

        /**
         * @brief Widens an integer point for drawing.
         *
         * Implicit on purpose: a caller may hand simulation state to a
         * renderer, and the absence of the reverse conversion is what
         * keeps float coordinates out of that state.
         */
        constexpr PointF(const Point point) noexcept
            : x(static_cast<float>(point.x)),
              y(static_cast<float>(point.y))
        {
        }

        [[nodiscard]] bool operator==(const PointF &other) const = default;
    };

}
