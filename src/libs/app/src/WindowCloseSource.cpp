#include "antwika/app/WindowCloseSource.hpp"

#include <antwika/engine/Events.hpp>
#include <antwika/event/ITickEventSource.hpp>

#include "antwika/app/WindowEvents.hpp"

namespace antwika::app
{

    using antwika::engine::events::kStop;
    using antwika::event::ITickEventSource;

    WindowCloseSource::WindowCloseSource(
        ITickEventSource &inner, IGfxBackend &backend, IWindow &window)
        : inner(inner), backend(backend), window(window)
    {
    }

    std::vector<Event> WindowCloseSource::eventsFor(antwika::time::Tick tick)
    {
        pumpEvents();

        auto events = inner.eventsFor(tick);

        // No guard against saying this twice.
        // StopSignal ends the loop on the tick carrying the first one.
        // And a second engine.stop would change nothing anyway.
        if (!window.isOpen())
        {
            events.push_back(Event{.name = kStop}); // GCOVR_EXCL_LINE
        }

        return events;
    } // GCOVR_EXCL_LINE

    void WindowCloseSource::pumpEvents()
    {
        if (closeRequestedOn(backend, window.id()))
        {
            window.close();
        }
    }

} // namespace antwika::app
