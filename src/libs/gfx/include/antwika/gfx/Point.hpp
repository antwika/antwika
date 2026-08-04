#pragma once

#include <antwika/geometry/Point.hpp>

namespace antwika::gfx
{

    /**
     * @brief antwika::geometry's Point, under the name every caller
     * here already uses.
     *
     * The type moved out so that a module wanting a rectangle does not
     * have to link a graphics library to get one -- antwika::replay
     * pulled in stb, glm and an embedded font for the sake of a width
     * and a height.
     * It is re-exported rather than renamed because `gfx::Point` is
     * what two hundred and seventy-eight files call it, and a rename
     * would have been churn with nothing at the end of it.
     */
    using antwika::geometry::Point;

} // namespace antwika::gfx
