#pragma once

#include "antwika/geometry/Point.hpp"
#include "antwika/geometry/Size.hpp"

namespace antwika::geometry
{

    struct Rect final
    {
        Point origin;
        Size size;

        [[nodiscard]] bool operator==(const Rect &other) const = default;
    };

}
