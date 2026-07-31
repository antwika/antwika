#pragma once

#include <optional>

#include <antwika/gfx/Point.hpp>

namespace antwika::ui
{

    using antwika::gfx::Point;

    /**
     * @brief Where a free-moving pointer is, for deciding appearance and
     * nothing else.
     *
     * A separate type from ui::Pointer, and deliberately a much smaller
     * one: it carries a position and it carries nothing a press could be
     * read out of. There is no `down` and no `pressed` here, so a hover
     * pointer cannot say that a button went down, cannot reach
     * Interactions, and cannot decide which widget was activated -- not
     * because a rule forbids it but because the value has no field that
     * could mean it.
     *
     * That is the whole point of the type existing. Hover is visual
     * candy: it may not affect what a run computes, and the cheapest way
     * to guarantee that is to make the thing that carries it incapable
     * of saying anything a run computes from.
     *
     * **Where the position comes from matters.** The one a UI wants here
     * is a free-moving one -- a position that arrives every frame,
     * including between clicks -- and in this codebase that is
     * antwika::input::PointerHintChannel, which is deliberately outside
     * the recorded event stream. A live run and its replay do not agree
     * on it, which is exactly why nothing but the picture may be a
     * function of it. See docs/hover-is-not-simulation.md.
     *
     * A default-constructed HoverPointer reports no pointer, and
     * applyHover() with one leaves a picture exactly as it was.
     */
    struct HoverPointer
    {
        /**
         * @brief Where the pointer is, in the same pixels the UI was
         * laid out in.
         *
         * Absent when nothing has reported a position yet, and for the
         * whole run under a backend with no pointer. Absent rather than
         * an origin, because an origin is a real place a widget can be.
         */
        std::optional<Point> position{};

        /**
         * @brief Compare two hover pointers.
         * @param other The hover pointer to compare against.
         * @return True when both report the same position.
         */
        [[nodiscard]] bool operator==(const HoverPointer &other) const =
            default;
    };

} // namespace antwika::ui
