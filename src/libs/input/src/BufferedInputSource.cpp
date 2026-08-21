#include "antwika/input/BufferedInputSource.hpp"

#include <iterator>
#include <utility>

#include <antwika/event/ITickEventSource.hpp>

namespace antwika::input
{

    using antwika::event::ITickEventSource;

    BufferedInputSource::BufferedInputSource(ITickEventSource &innerSource)
        : inner(innerSource)
    {
    }

    void BufferedInputSource::pollFrame(antwika::time::Tick tick)
    {
        auto events = inner.eventsFor(tick);

        pendingEvents.insert(
            pendingEvents.end(),
            std::make_move_iterator(events.begin()),
            std::make_move_iterator(events.end()));
    }

    std::vector<Event> BufferedInputSource::eventsFor(
        antwika::time::Tick tick)
    {
        pollFrame(tick);

        return std::exchange(pendingEvents, {});
    }

}
