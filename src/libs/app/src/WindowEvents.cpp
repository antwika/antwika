#include "antwika/app/WindowEvents.hpp"

#include <variant>

#include <antwika/gfx/WindowEvent.hpp>

namespace antwika::app
{

    using antwika::gfx::CloseRequested;
    using antwika::gfx::Resized;

    WindowChanges windowChanges(
        IGfxBackend &backend, const WindowId window)
    {
        WindowChanges changes;

        while (const auto event = backend.pollEvent())
        {
            if (event->window != window)
            {
                continue;
            }

            if (std::holds_alternative<CloseRequested>(event->payload))
            {
                changes.closeRequested = true;
            }

            if (const auto *resized =
                    std::get_if<Resized>(&event->payload))
            {
                changes.resizedSize = resized->size;
            }
        }

        return changes;
    }

    bool closeRequestedOn(IGfxBackend &backend, const WindowId window)
    {
        return windowChanges(backend, window).closeRequested;
    }

}
