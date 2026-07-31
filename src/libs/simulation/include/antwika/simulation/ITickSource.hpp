#pragma once

#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::simulation
{

    using antwika::event::Event;

    /**
     * @brief Supplies the events that occurred on a given tick, whether from
     * a live source or a loaded replay.
     *
     * **Ticks are asked for once each, in increasing order.** EngineLoop
     * counts up from zero and asks once per tick, and an implementation
     * may rely on that: ReplaySource walks its recording with a cursor,
     * IdleMotionSource carries a latched movement forward from the tick
     * it arrived on. Asking twice, or going back, is not a question any
     * of them can answer.
     */
    class ITickSource
    {
    public:
        virtual ~ITickSource() = default;

        /**
         * @brief Get the events that occurred on a given tick.
         * @param tick The tick to fetch events for, greater than the tick
         * of any previous call.
         * @return The events for that tick, in original order.
         */
        [[nodiscard]] virtual std::vector<Event> eventsFor(
            antwika::time::Tick tick) = 0;
    };

} // namespace antwika::simulation
