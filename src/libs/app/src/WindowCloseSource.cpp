#include "antwika/app/WindowCloseSource.hpp"

#include <variant>

#include <antwika/engine/Events.hpp>
#include <antwika/event/ITickEventSource.hpp>
#include <antwika/gfx/WindowEvent.hpp>

namespace antwika::app
{

    using antwika::engine::events::kStop;
    using antwika::event::ITickEventSource;
    using antwika::gfx::CloseRequested;

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
        while (const auto event = backend.pollEvent())
        {
            // The backend pumps one queue for all its windows.
            // An event for somebody else's window is not ours.
            if (event->window != window.id())
            {
                continue;
            }

            if (std::holds_alternative<CloseRequested>(event->payload))
            {
                window.close();
            }
        }
    }

} // namespace antwika::app
