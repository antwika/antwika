#pragma once

#include <optional>

#include <antwika/gfx/Point.hpp>

namespace antwika::ui
{

    using antwika::gfx::Point;

    /**
     * @brief What the caller reports about the pointer, for one frame.
     *
     * This library reads no device: an application folds the edges its
     * input backend reports -- which is what antwika::input::InputState is
     * for -- and hands the result across as a value. That is what keeps
     * antwika::ui a leaf on antwika::gfx, and what lets a test drive a
     * button by writing a literal.
     *
     * A default-constructed Pointer reports no pointer at all, so a UI
     * built without one behaves exactly as it did before there was one.
     */
    struct Pointer
    {
        /**
         * @brief Where the pointer is, in the same pixels the UI is laid
         * out in.
         *
         * Absent when nothing has reported a position yet, and for the
         * whole run under a backend with no pointer. Absent rather than
         * an origin, because an origin is a real place a widget can be.
         */
        std::optional<Point> position{};

        /**
         * @brief Whether a pointer button is being held.
         */
        bool down = false;

        /**
         * @brief Whether a pointer button went down for this frame.
         *
         * A widget activates on the press rather than on a release
         * matched to it, so that nothing has to be remembered between
         * frames: matching the two would be cross-frame state a replay
         * would then have to regenerate.
         */
        bool pressed = false;
    };

} // namespace antwika::ui
