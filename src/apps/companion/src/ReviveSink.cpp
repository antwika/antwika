#include "antwika/companion/ReviveSink.hpp"

#include <variant>

#include <antwika/input/InputEvent.hpp>
#include <antwika/input/MouseButton.hpp>

#include "antwika/companion/PetLayout.hpp"

namespace antwika::companion
{

    using antwika::input::MouseButton;
    using antwika::input::PointerButtonPressed;

    ReviveSink::ReviveSink(
        Pet &pet,
        Lineage &lineage,
        const IInputEventCodec &codec,
        const Size canvas)
        : pet(pet), lineage(lineage), codec(codec), canvas(canvas)
    {
    }

    void ReviveSink::handle(const TickEvent &event)
    {
        // The button is only there while there is nothing else to do.
        // A press on that part of a living companion's window is a prod.
        // Which PropSink has already answered by the time this runs.
        if (pet.state() != PetState::Perished)
        {
            return;
        }

        const auto edge = codec.decode(event.event);

        if (!edge)
        {
            return;
        }

        const auto *pressed = std::get_if<PointerButtonPressed>(&*edge);

        if (pressed == nullptr || pressed->button != MouseButton::Left)
        {
            return;
        }

        if (!withinReviveButton(
                canvas,
                Point{
                    .x = pressed->position.x,
                    .y = pressed->position.y}))
        {
            return;
        }

        // Offered to the record before the companion is replaced.
        // Afterwards there is nothing left to ask how long it lived.
        lineage.record(pet.ticks());
        lineage.advance();
        pet.revive();
    }

} // namespace antwika::companion
