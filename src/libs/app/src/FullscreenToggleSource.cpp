#include "antwika/app/FullscreenToggleSource.hpp"

#include <variant>

#include <antwika/event/ITickEventSource.hpp>
#include <antwika/input/InputEvent.hpp>

namespace antwika::app
{

    using antwika::event::ITickEventSource;

    FullscreenToggleSource::FullscreenToggleSource(
        ITickEventSource &innerSource,
        IWindow &window,
        const IInputEventCodec &codec,
        Key key)
        : inner(innerSource), window(window), codec(codec), key(key)
    {
    }

    std::vector<Event> FullscreenToggleSource::eventsFor(
        antwika::time::Tick tick)
    {
        auto events = inner.eventsFor(tick);

        for (const auto &event : events)
        {
            const auto decodedEvent = codec.decode(event);

            if (!decodedEvent.has_value())
            {
                continue;
            }

            const auto *pressedEvent =
                std::get_if<antwika::input::KeyPressed>(&*decodedEvent);

            if (pressedEvent != nullptr && pressedEvent->key == key
                && !pressedEvent->repeat)
            {
                window.setFullscreen(!window.isFullscreen());
            }
        }

        return events;
    }

}
