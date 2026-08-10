#pragma once

#include "antwika/geometry/PointF.hpp"
#include "antwika/geometry/Rect.hpp"
#include "antwika/geometry/SizeF.hpp"

namespace antwika::geometry
{

    struct RectF final
    {
        PointF origin;
        SizeF size;

        constexpr RectF() noexcept = default;

        constexpr RectF(const PointF origin, const SizeF size) noexcept
            : origin(origin), size(size)
        {
        }

        /**
         * @brief Widens an integer rectangle for drawing.
         *
         * Implicit on purpose, for the reason given on PointF.
         */
        constexpr RectF(const Rect rect) noexcept
            : origin(rect.origin), size(rect.size)
        {
        }

        [[nodiscard]] bool operator==(const RectF &other) const = default;
    };

}
