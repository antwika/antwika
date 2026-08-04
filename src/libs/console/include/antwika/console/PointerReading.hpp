#pragma once

#include <variant>

#include <antwika/gfx/Point.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/Position.hpp>

namespace antwika::console
{

    using antwika::gfx::Point;
    using antwika::input::InputEvent;
    using antwika::input::Position;

    /**
     * @brief Read an input position as a point on the canvas.
     *
     * input::Position and gfx::Point match field for field. They stay
     * unrelated types so input need not depend on gfx. Deciding they mean
     * the same thing is the application's job, and this is the
     * application saying so -- once, for every sink in this app that has
     * to say it, since a second copy is a second place to say it
     * differently.
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
     * A scroll does not qualify, which is the subtle one -- it carries no
     * position of its own.
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

} // namespace antwika::console
