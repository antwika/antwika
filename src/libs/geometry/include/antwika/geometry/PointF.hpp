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

        constexpr PointF(const Point point) noexcept
            : x(static_cast<float>(point.x)),
              y(static_cast<float>(point.y))
        {
        }

        [[nodiscard]] bool operator==(const PointF &other) const = default;
    };

}
