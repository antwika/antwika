#pragma once

#include <variant>

#include <antwika/gfx/Point.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputState.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/input/Position.hpp>
#include <antwika/ui/Pointer.hpp>

namespace antwika::app
{

    using antwika::gfx::Point;
    using antwika::input::InputEvent;
    using antwika::input::InputState;
    using antwika::input::MouseButton;
    using antwika::input::Position;
    using antwika::ui::Pointer;

    /**
     * @brief Read an input position as a point on the canvas.
     *
     * input::Position and gfx::Point match field for field. They stay
     * unrelated types so antwika::input need not depend on antwika::gfx.
     * Deciding they mean the same thing is the application's job, and
     * this is where an application says so -- once, rather than once per
     * app, which is once per place to say it differently.
     *
     * @param position The position an input event reported.
     * @return The same place, as a point.
     */
    [[nodiscard]] constexpr Point asPoint(Position position) noexcept
    {
        return Point{.x = position.x, .y = position.y};
    }

    /**
     * @brief Check whether an event says where the pointer is.
     *
     * Until something says, the pointer is nowhere: the folded default
     * would put it in the canvas's corner, and a widget can be in that
     * corner and would look hovered. So a caller holding a
     * ui::Pointer::position must leave it unset until this is true of
     * some event it has folded.
     *
     * A scroll does not qualify, which is the subtle one -- it carries
     * no position of its own.
     *
     * @param event The event about to be folded.
     * @return True when the event carries a pointer position.
     */
    [[nodiscard]] constexpr bool locates(const InputEvent &event) noexcept
    {
        using antwika::input::PointerButtonPressed;
        using antwika::input::PointerButtonReleased;
        using antwika::input::PointerMoved;

        return std::holds_alternative<PointerMoved>(event)
               || std::holds_alternative<PointerButtonPressed>(event)
               || std::holds_alternative<PointerButtonReleased>(event);
    }

    /**
     * @brief Read folded input state as the pointer a UI frame wants.
     *
     * This is the whole reason this module exists: antwika::ui reads no
     * device and antwika::input knows nothing about a UI, deliberately,
     * so somebody above both has to say that one describes the other.
     * Doing it here rather than in either library is what keeps neither
     * depending on the other.
     *
     * @param state Both devices, folded for this tick.
     * @param located Whether anything has said where the pointer is yet;
     * see locates().
     * @param button The button a widget activates on.
     * @return What the UI should be told about the pointer.
     */
    [[nodiscard]] Pointer pointerFrom(
        const InputState &state,
        bool located,
        MouseButton button = MouseButton::Left) noexcept;

} // namespace antwika::app
