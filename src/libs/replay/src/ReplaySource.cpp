#include "antwika/replay/ReplaySource.hpp"

#include <algorithm>
#include <utility>

namespace antwika::replay
{

    ReplaySource::ReplaySource(std::vector<TickEvent> events)
        : events(std::move(events))
    {
        // Sorted once here so that eventsFor can walk forwards only.
        // Stable, so a shared tick keeps its written order.
        // That order is the order those events were dispatched in.
        std::stable_sort(
            this->events.begin(),
            this->events.end(),
            [](const TickEvent &left, const TickEvent &right)
            { return left.tick < right.tick; });
    }

    std::vector<Event> ReplaySource::eventsFor(antwika::time::Tick tick)
    {
        // What is still behind the asked-for tick has gone by unasked.
        while (cursor < events.size() && events[cursor].tick < tick)
        {
            ++cursor;
        }

        std::vector<Event> result;

        while (cursor < events.size() && events[cursor].tick == tick)
        {
            result.push_back(std::move(events[cursor].event));
            ++cursor;
        }

        return result;
    } // GCOVR_EXCL_LINE

} // namespace antwika::replay
