#pragma once

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>

#include "antwika/companion/Pet.hpp"

namespace antwika::companion
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;

    /**
     * @brief Steps the companion once per engine.tick.
     *
     * One engine tick is one tick of its life, the way one tick is one
     * step of the battle in apps/tower_defence.
     * It defines no event of its own: getting hungrier, falling asleep
     * and perishing are all regenerated from the tick, so none of them
     * is ever persisted.
     */
    class PetSink final : public ITickEventSink
    {
    public:
        /**
         * @brief Construct the sink over the companion it steps.
         * @param pet Stepped once per tick. Must outlive this sink.
         */
        explicit PetSink(Pet &pet);

        PetSink(const PetSink &) = delete;
        PetSink(PetSink &&) = delete;

        PetSink &operator=(const PetSink &) = delete;
        PetSink &operator=(PetSink &&) = delete;

        /**
         * @brief Step the companion if this is a tick.
         * @param event The event to fold in; anything but engine.tick is
         * ignored.
         */
        void handle(const TickEvent &event) override;

    private:
        Pet &pet;
    };

} // namespace antwika::companion
