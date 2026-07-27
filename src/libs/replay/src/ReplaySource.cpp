#include "antwika/replay/ReplaySource.hpp"

#include <utility>

namespace antwika::replay
{

    ReplaySource::ReplaySource(std::vector<TimedEvent> events) : events(std::move(events))
    {
    }

    std::vector<Event> ReplaySource::eventsFor(antwika::time::Tick tick)
    {
        std::vector<Event> result;

        for (const auto &timedEvent : events)
        {
            if (timedEvent.tick == tick)
            {
                result.push_back(timedEvent.event);
            }
        }

        return result;
    } // GCOVR_EXCL_LINE

} // namespace antwika::replay
