#pragma once

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>

#include "antwika/tower_defence/Battle.hpp"

namespace antwika::tower_defence
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;

    /**
     * @brief Steps the battle once per engine.tick.
     *
     * One engine tick is one step of the battle, the way one tick is one
     * step of the poker loop.
     * It defines no event of its own: a step is regenerated from the
     * tick, so nothing about it is ever persisted.
     */
    class BattleSink final : public ITickEventSink
    {
    public:
        /**
         * @brief Construct the sink over the battle it steps.
         * @param battle Stepped once per tick. Must outlive this sink.
         */
        explicit BattleSink(Battle &battle);

        BattleSink(const BattleSink &) = delete;
        BattleSink(BattleSink &&) = delete;

        BattleSink &operator=(const BattleSink &) = delete;
        BattleSink &operator=(BattleSink &&) = delete;

        /**
         * @brief Step the battle if this is a tick.
         * @param event The event to fold in; anything but engine.tick is
         * ignored.
         */
        void handle(const TickEvent &event) override;

    private:
        Battle &battle;
    };

} // namespace antwika::tower_defence
