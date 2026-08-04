#include "antwika/app/WindowEvents.hpp"

#include <variant>

#include <antwika/gfx/WindowEvent.hpp>

namespace antwika::app
{

    using antwika::gfx::CloseRequested;

    bool closeRequestedOn(IGfxBackend &backend, const WindowId window)
    {
        bool requested = false;

        while (const auto event = backend.pollEvent())
        {
            // The backend pumps one queue for all its windows.
            // An event for somebody else's window is not ours.
            if (event->window != window)
            {
                continue;
            }

            if (std::holds_alternative<CloseRequested>(event->payload))
            {
                requested = true;
            }
        }

        return requested;
    }

} // namespace antwika::app
