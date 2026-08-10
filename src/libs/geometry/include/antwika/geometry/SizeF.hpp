#pragma once

#include "antwika/geometry/Size.hpp"

namespace antwika::geometry
{

    struct SizeF final
    {
        float width = 0.0F;
        float height = 0.0F;

        constexpr SizeF() noexcept = default;

        constexpr SizeF(const float width, const float height) noexcept
            : width(width), height(height)
        {
        }

        /**
         * @brief Widens an integer size for drawing.
         *
         * Implicit on purpose, for the reason given on PointF.
         */
        constexpr SizeF(const Size size) noexcept
            : width(static_cast<float>(size.width)),
              height(static_cast<float>(size.height))
        {
        }

        [[nodiscard]] bool operator==(const SizeF &other) const = default;
    };

}
