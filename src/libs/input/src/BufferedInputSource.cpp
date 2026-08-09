#include "antwika/input/BufferedInputSource.hpp"

#include <iterator>
#include <utility>

#include <antwika/event/ITickEventSource.hpp>

namespace antwika::input
{

    using antwika::event::ITickEventSource;

    BufferedInputSource::BufferedInputSource(ITickEventSource &inner)
        : inner(inner)
    {
    }

    void BufferedInputSource::pump(antwika::time::Tick tick)
    {
        auto events = inner.eventsFor(tick);

        pending.insert(
            pending.end(),
            std::make_move_iterator(events.begin()),
            std::make_move_iterator(events.end()));
    }

    std::vector<Event> BufferedInputSource::eventsFor(
        antwika::time::Tick tick)
    {
        pump(tick);

        return std::exchange(pending, {});
    }

}
