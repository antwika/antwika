#pragma once

#include <cstddef>
#include <vector>

#include <antwika/event/TickEvent.hpp>

#include "IReplaySource.hpp"

namespace antwika::replay
{

    using antwika::event::TickEvent;

    /**
     * @brief IReplaySource that feeds a recorded or hand-scripted sequence of
     * TickEvents back out, one tick's worth at a time, in original order.
     *
     * The recording is walked once, not once per tick. A cursor advances
     * through it as ticks are asked for, which is what keeps replaying a
     * session linear in its length rather than quadratic -- for an app
     * whose input and its running time both grow with the session, the
     * difference is the whole cost of a long replay.
     *
     * That rests on IReplaySource's contract that ticks are asked for
     * once each and in increasing order. The constructor sorts, so a
     * hand-authored file whose ticks are not in order still replays; what
     * it cannot survive is a *caller* going backwards.
     *
     * Each event is handed out rather than copied, so a payload string is
     * moved out of the recording the one time it is asked for.
     */
    class ReplaySource final : public IReplaySource
    {
    public:
        /**
         * @brief Construct the source from a full sequence of tick events.
         * @param events The events to replay; sorted by tick, keeping the
         * order of anything sharing one.
         */
        explicit ReplaySource(std::vector<TickEvent> events);

        /**
         * @brief Get the events that occurred on a given tick.
         * @param tick The tick to fetch events for, greater than the tick
         * of any previous call.
         * @return The events for that tick, in original order, moved out
         * of the recording.
         */
        [[nodiscard]] std::vector<Event> eventsFor(
            antwika::time::Tick tick) override;

    private:
        std::vector<TickEvent> events;

        // How far through the recording the ticks asked for have got.
        // Everything before it has been handed out or gone by unasked.
        std::size_t cursor = 0;
    };

} // namespace antwika::replay
