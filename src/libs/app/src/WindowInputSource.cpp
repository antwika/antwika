#include "antwika/app/WindowInputSource.hpp"

#include <antwika/engine/Events.hpp>
#include <antwika/event/ITickEventSource.hpp>

#include "antwika/app/WindowEvents.hpp"

namespace antwika::app
{

    using antwika::engine::events::kStop;
    using antwika::event::ITickEventSource;

    WindowInputSource::WindowInputSource(
        ITickEventSource &inner, IGfxBackend &backend, WindowId window)
        : inner(inner), backend(backend), window(window)
    {
    }

    std::vector<Event> WindowInputSource::eventsFor(antwika::time::Tick tick)
    {
        auto events = inner.eventsFor(tick);

        const bool closeRequested = closeRequestedOn(backend, window);

        if (closeRequested)
        {
            events.push_back(Event{.name = kStop}); // GCOVR_EXCL_LINE
        }

        return events;
    } // GCOVR_EXCL_LINE

}
