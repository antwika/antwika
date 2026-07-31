#pragma once

#include <cstdint>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>

namespace antwika::ui::detail
{

    using antwika::gfx::Point;
    using antwika::gfx::Rect;

    /**
     * @brief Check whether a point falls inside a rectangle.
     *
     * The one hit-test in the library, shared by the pointer resolving
     * inside the tick path and the hover pass repainting a finished
     * picture. Two copies would be two answers the moment either grew an
     * edge case, and which widget a press lands on and which one lights
     * up have to be the same question.
     *
     * @param rect The area to test against.
     * @param point The place to test.
     * @return True when the point is inside, edges included on the top
     * and left and excluded on the bottom and right.
     */
    [[nodiscard]] inline bool contains(
        const Rect &rect, Point point) noexcept
    {
        // A right edge is an int32 origin plus a uint32 extent.
        // That is exactly the sum that wraps, hence 64 bits.
        const auto left = static_cast<std::int64_t>(rect.origin.x);
        const auto top = static_cast<std::int64_t>(rect.origin.y);
        const auto x = static_cast<std::int64_t>(point.x);
        const auto y = static_cast<std::int64_t>(point.y);

        // Half-open, so two touching rectangles cannot both be hit.
        // A collapsed one is therefore hit by nothing.
        return x >= left && x < left + rect.size.width && y >= top
               && y < top + rect.size.height;
    }

} // namespace antwika::ui::detail
