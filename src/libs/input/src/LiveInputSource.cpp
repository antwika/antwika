#include "antwika/input/LiveInputSource.hpp"

namespace antwika::input
{

    LiveInputSource::LiveInputSource(
        ITickSource &inner,
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

        // The closing brace below is where the vector's destructor runs.
        // Only an unwind out of this function ever reaches it.
        return events;
    } // GCOVR_EXCL_LINE

} // namespace antwika::input
