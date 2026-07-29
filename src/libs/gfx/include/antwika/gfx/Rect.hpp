#pragma once

#include "antwika/gfx/Point.hpp"
#include "antwika/gfx/Size.hpp"

namespace antwika::gfx
{

    /**
     * @brief An axis-aligned rectangle, given by its top-left corner.
     */
    struct Rect
    {
        Point origin;
        Size size;
    };

} // namespace antwika::gfx
