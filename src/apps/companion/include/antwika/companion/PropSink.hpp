#pragma once

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/input/IInputEventCodec.hpp>

#include "antwika/companion/Pet.hpp"

namespace antwika::companion
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;
    using antwika::gfx::Size;
    using antwika::input::IInputEventCodec;

    /**
     * @brief Turns a left press into whichever verb the prop under it
     * names, inside the tick path.
     *
     * **This application defines no event for feeding, playing with,
     * putting to bed or annoying the companion, and that is the point.**
     * A `--record` run persists the press; which prop it landed on, and
     * whether the companion was in a state to answer it, are worked out
     * again on replay from the same press against the same canvas and
     * the same tick count.
     * Persisting the meal as well would feed it twice per press, the
     * same trap td::TowerPlacementSink describes for building a tower.
     *
     * Where each prop is is `propAt()`'s answer, which is `propBox()`'s
     * answer, which is what PetScene paints -- so what somebody sees and
     * what they can press are one set of rectangles rather than two that
     * agree today.
     * The canvas it resolves against is the *configured* window size,
     * never the size a window reports, for the reason
     * life::PointerToggleSink gives about cells.
     *
     * A press that lands on no prop is `Pet::pester()` rather than
     * nothing at all: sloppy aim has a price, which is what leaves the
     * props worth aiming at.
     * A press of any kind while the companion is asleep wakes it, and
     * that rule lives in `Pet` rather than here -- this sink says where
     * a press landed, and the companion says what it was in a state to
     * do about it.
     *
     * Registered before PetSink, so a press is answered by the state the
     * last tick ended with and the step that follows sees the meal.
     */
    class PropSink final : public ITickEventSink
    {
    public:
        /**
         * @brief Construct the sink over its collaborators.
         * @param pet Receives the verbs. Must outlive this sink.
         * @param codec Decodes each event. Must outlive this sink.
         * @param canvas The size the props are laid out against.
         */
        PropSink(Pet &pet, const IInputEventCodec &codec, Size canvas);

        PropSink(const PropSink &) = delete;
        PropSink(PropSink &&) = delete;

        PropSink &operator=(const PropSink &) = delete;
        PropSink &operator=(PropSink &&) = delete;

        /**
         * @brief Answer this press, if it is one.
         * @param event The event to fold in; anything that is not a left
         * pointer press is ignored.
         * @throws antwika::input::InputError If the event is one of
         * antwika::input's but its payload is malformed.
         */
        void handle(const TickEvent &event) override;

    private:
        Pet &pet;
        const IInputEventCodec &codec;
        Size canvas;
    };

} // namespace antwika::companion
