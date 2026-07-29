#pragma once

#include <vector>

#include <antwika/event/TickEvent.hpp>

#include "IReplaySource.hpp"

namespace antwika::replay
{

    using antwika::event::TickEvent;

    /**
     * @brief IReplaySource that feeds a recorded or hand-scripted sequence of
     * TickEvents back out, one tick's worth at a time, in original order.
     */
    class ReplaySource final : public IReplaySource
    {
    public:
        /**
         * @brief Construct the source from a full sequence of tick events.
         * @param events The events to replay, in original order.
         */
        explicit ReplaySource(std::vector<TickEvent> events);

        /**
         * @brief Get the events that occurred on a given tick.
         * @param tick The tick to fetch events for.
         * @return The events for that tick, in original order.
         */
        [[nodiscard]] std::vector<Event> eventsFor(
            antwika::time::Tick tick) override;

    private:
        std::vector<TickEvent> events;
    };

} // namespace antwika::replay
