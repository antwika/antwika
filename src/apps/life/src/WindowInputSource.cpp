#include "antwika/life/WindowInputSource.hpp"

#include <variant>

#include <antwika/engine/Events.hpp>
#include <antwika/gfx/WindowEvent.hpp>

namespace antwika::life
{

    using antwika::gfx::CloseRequested;

    WindowInputSource::WindowInputSource(
        IReplaySource &inner, IGfxBackend &backend, WindowId window)
        : inner(inner), backend(backend), window(window)
    {
    }

    std::vector<Event> WindowInputSource::eventsFor(antwika::time::Tick tick)
    {
        auto events = inner.eventsFor(tick);

        bool closeRequested = false;
        while (const auto event = backend.pollEvent())
        {
            if (event->window != window)
            {
                continue;
            }

            if (std::holds_alternative<CloseRequested>(event->payload))
            {
                closeRequested = true;
            }
        }

        // One stop, however many times closing was asked for.
        if (closeRequested)
        {
            events.push_back(
                Event{.name = antwika::engine::events::kStop});
        }

        return events;
    } // GCOVR_EXCL_LINE

} // namespace antwika::life
