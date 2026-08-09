#include "antwika/input/LiveInputSource.hpp"
#include <antwika/event/ITickEventSource.hpp>

namespace antwika::input
{

    using antwika::event::ITickEventSource;

    LiveInputSource::LiveInputSource(
        ITickEventSource &inner,
        IInputBackend &backend,
        const IInputEventCodec &codec)
        : inner(inner), backend(backend), codec(codec)
    {
    }

    std::vector<Event> LiveInputSource::eventsFor(antwika::time::Tick tick)
    {
        auto events = inner.eventsFor(tick);

        while (const auto edge = backend.pollEvent())
        {
            events.push_back(codec.encode(*edge));
        }

        return events;
    } // GCOVR_EXCL_LINE

}
