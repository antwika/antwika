#pragma once

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/input/IInputEventCodec.hpp>

#include "antwika/companion/Lineage.hpp"
#include "antwika/companion/Pet.hpp"

namespace antwika::companion
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;
    using antwika::gfx::Size;
    using antwika::input::IInputEventCodec;

    /**
     * @brief Turns a left press on the "new companion" button into a new
     * companion, inside the tick path.
     *
     * **This application defines no event for starting a new companion
     * either, and that is the same point PropSink makes.** A `--record`
     * run persists the press; that it landed on the button, and that
     * there was a button to land on, are worked out again on replay from
     * the same press against the same canvas and the same state.
     * Persisting the revival as well would start two companions per
     * press.
     *
     * Where the button is is reviveButtonRect()'s answer, which is also
     * what PetScene paints, so what somebody sees and what they can
     * press are one rectangle rather than two that agree today.
     *
     * **The lineage is written here rather than by `Pet::revive()`**,
     * because a companion cannot record its own succession: `revive()`
     * replaces the companion whole, so anything it was meant to carry
     * forward would go with it. The age that has just ended is offered
     * to the record before the companion is replaced, and the
     * generation moves on after -- in that order, since afterwards
     * there is nothing left to ask how long it lived.
     *
     * The canvas it tests against is the *configured* window size, never
     * the size a window reports, for the reason life::PointerToggleSink
     * gives about cells: a hit-test is a function of the layout, and a
     * resized window would resolve a recorded press differently.
     *
     * **Registered after PropSink**, which is what keeps one press from
     * meaning two things: a press on the button reaches PropSink first,
     * where a perished companion ignores it, and only then reaches this.
     * The other order would revive first and then prod the new
     * companion with a press nobody aimed at it.
     */
    class ReviveSink final : public ITickEventSink
    {
    public:
        /**
         * @brief Construct the sink over its collaborators.
         * @param pet Started again. Must outlive this sink.
         * @param lineage Records what has ended. Must outlive this sink.
         * @param codec Decodes each event. Must outlive this sink.
         * @param canvas The size the button is laid out against.
         */
        ReviveSink(
            Pet &pet,
            Lineage &lineage,
            const IInputEventCodec &codec,
            Size canvas);

        ReviveSink(const ReviveSink &) = delete;
        ReviveSink(ReviveSink &&) = delete;

        ReviveSink &operator=(const ReviveSink &) = delete;
        ReviveSink &operator=(ReviveSink &&) = delete;

        /**
         * @brief Start a new companion if this press asked for one.
         * @param event The event to fold in; anything that is not a left
         * press on the button of a perished companion is ignored.
         * @throws antwika::input::InputError If the event is one of
         * antwika::input's but its payload is malformed.
         */
        void handle(const TickEvent &event) override;

    private:
        Pet &pet;
        Lineage &lineage;
        const IInputEventCodec &codec;
        Size canvas;
    };

} // namespace antwika::companion
