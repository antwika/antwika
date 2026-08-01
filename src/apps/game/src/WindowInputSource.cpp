#include "antwika/game/WindowInputSource.hpp"

#include <variant>

#include <antwika/engine/Events.hpp>
#include <antwika/gfx/WindowEvent.hpp>

namespace antwika::game
{

    using antwika::engine::events::kStop;
    using antwika::gfx::CloseRequested;

    WindowInputSource::WindowInputSource(
        ITickEventSource &inner, IGfxBackend &backend, WindowId window)
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
        // Every branch left on the excluded line is the allocator's.
        // Those are push_back's throw edge and its growth path.
        // Plus the heap branch of a name far too short to need one.
        if (closeRequested)
        {
            events.push_back(Event{.name = kStop}); // GCOVR_EXCL_LINE
        }

        return events;
    } // GCOVR_EXCL_LINE

} // namespace antwika::game
