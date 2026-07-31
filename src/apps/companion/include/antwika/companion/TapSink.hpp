#pragma once

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/input/IInputEventCodec.hpp>

#include "antwika/companion/Pet.hpp"

namespace antwika::companion
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;
    using antwika::input::IInputEventCodec;

    /**
     * @brief Turns a left press on the window into a tap, inside the
     * tick path.
     *
     * **This application defines no event for feeding or for waking the
     * companion up, and that is the point.**
     * A `--record` run persists the press; whether it landed on a hungry
     * companion, a sleeping one or a perished one is worked out again on
     * replay from the same press and the same tick count.
     * Persisting the meal as well would feed it twice per tap, the same
     * trap td::TowerPlacementSink describes for building a tower.
     *
     * There is deliberately no layout here and no canvas to hit-test
     * against. The window is 128 pixels square and holds one animal, so
     * a press anywhere in it means the same thing -- which is also why
     * nothing somebody sees can drift out of step with what they can
     * hit, since there is nothing to keep in step.
     *
     * Registered before PetSink, so a tap is answered by the state the
     * last tick ended with and the step that follows sees the meal.
     */
    class TapSink final : public ITickEventSink
    {
    public:
        /**
         * @brief Construct the sink over its collaborators.
         * @param pet Receives the taps. Must outlive this sink.
         * @param codec Decodes each event. Must outlive this sink.
         */
        TapSink(Pet &pet, const IInputEventCodec &codec);

        TapSink(const TapSink &) = delete;
        TapSink(TapSink &&) = delete;

        TapSink &operator=(const TapSink &) = delete;
        TapSink &operator=(TapSink &&) = delete;

        /**
         * @brief Tap the companion if this is a left press.
         * @param event The event to fold in; anything that is not a left
         * pointer press is ignored.
         * @throws antwika::input::InputError If the event is one of
         * antwika::input's but its payload is malformed.
         */
        void handle(const TickEvent &event) override;

    private:
        Pet &pet;
        const IInputEventCodec &codec;
    };

} // namespace antwika::companion
