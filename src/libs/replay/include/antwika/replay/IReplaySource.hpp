#pragma once

#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::replay
{

    using antwika::event::Event;

    /**
     * @brief Supplies the events that occurred on a given tick, whether from
     * a live source or a loaded replay.
     */
    class IReplaySource
    {
    public:
        virtual ~IReplaySource() = default;

        /**
         * @brief Get the events that occurred on a given tick.
         * @param tick The tick to fetch events for.
         * @return The events for that tick, in original order.
         */
        [[nodiscard]] virtual std::vector<Event> eventsFor(
            antwika::time::Tick tick) = 0;
    };

} // namespace antwika::replay
