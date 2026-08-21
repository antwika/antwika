#pragma once

#include "antwika/geometry/PointF.hpp"
#include "antwika/geometry/Rect.hpp"
#include "antwika/geometry/SizeF.hpp"

namespace antwika::geometry
{

    struct RectF final
    {
        PointF originPoint;
        SizeF size;

        constexpr RectF() noexcept = default;

        constexpr RectF(const PointF originPoint, const SizeF size) noexcept
            : originPoint(originPoint), size(size)
        {
        }

        constexpr RectF(const Rect rect) noexcept
            : originPoint(rect.originPoint), size(rect.size)
        {
        }

        [[nodiscard]] bool operator==(const RectF &other) const = default;
    };

    [[nodiscard]] constexpr bool holds(
        const RectF whereRect, const PointF point) noexcept
    {
        return point.x >= whereRect.originPoint.x
               && point.y >= whereRect.originPoint.y
               && point.x <= whereRect.originPoint.x + whereRect.size.width
               && point.y <= whereRect.originPoint.y + whereRect.size.height;
    }

}
