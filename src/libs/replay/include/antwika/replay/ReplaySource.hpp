#pragma once

#include <vector>

#include <antwika/event/TimedEvent.hpp>

#include "IReplaySource.hpp"

namespace antwika::replay
{

    using antwika::event::TimedEvent;

    // Feeds a recorded or hand-scripted sequence of TimedEvents back out.
    // It returns one tick's worth at a time, in original order.
    class ReplaySource final : public IReplaySource
    {
    public:
        explicit ReplaySource(std::vector<TimedEvent> events);

        [[nodiscard]] std::vector<Event> eventsFor(antwika::time::Tick tick) override;

    private:
        std::vector<TimedEvent> events;
    };

} // namespace antwika::replay
