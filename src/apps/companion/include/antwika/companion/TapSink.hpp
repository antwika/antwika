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
     * This sink hit-tests nothing, and it is the only one here that
     * does not. The window holds one animal, so a press anywhere in it
     * means a tap and every tap means the same thing -- there is no
     * region of the window this sink has to know about.
     *
     * The app does have a layout, and one press whose meaning depends
     * on where it landed: the "new companion" button a perished one is
     * offered. That is PetLayout's box and ReviveSink's hit-test, both
     * against the configured window size, and the two share the one
     * reviveButtonRect() so what somebody sees and what they can hit
     * cannot drift. ReviveSink is registered after this sink, so one
     * press can never be a tap and a revival both.
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
