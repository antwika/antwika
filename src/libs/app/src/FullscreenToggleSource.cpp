#include "antwika/app/FullscreenToggleSource.hpp"

#include <variant>

#include <antwika/input/InputEvent.hpp>

namespace antwika::app
{

    FullscreenToggleSource::FullscreenToggleSource(
        ITickEventSource &inner,
        IWindow &window,
        const IInputEventCodec &codec,
        Key key)
        : inner(inner), window(window), codec(codec), key(key)
    {
    }

    std::vector<Event> FullscreenToggleSource::eventsFor(
        antwika::time::Tick tick)
    {
        auto events = inner.eventsFor(tick);

        for (const auto &event : events)
        {
            const auto decoded = codec.decode(event);

            if (!decoded.has_value())
            {
                continue;
            }

            const auto *pressed =
                std::get_if<antwika::input::KeyPressed>(&*decoded);

            if (pressed != nullptr && pressed->key == key
                && !pressed->repeat)
            {
                window.setFullscreen(!window.isFullscreen());
            }
        }

        // Untouched, which is the whole guarantee of this class.
        return events;
    }

} // namespace antwika::app
