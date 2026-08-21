#include "antwika/input/StopOnKeySource.hpp"

#include <variant>

#include <antwika/engine/Events.hpp>
#include <antwika/event/ITickEventSource.hpp>

#include "antwika/input/InputEvent.hpp"

namespace antwika::input
{

    using antwika::engine::events::kStop;
    using antwika::event::ITickEventSource;

    StopOnKeySource::StopOnKeySource(
        ITickEventSource &innerSource, const IInputEventCodec &codec, Key key)
        : inner(innerSource), codec(codec), key(key)
    {
    }

    std::vector<Event> StopOnKeySource::eventsFor(antwika::time::Tick tick)
    {
        auto events = inner.eventsFor(tick);

        bool stopping = false;
        for (const auto &event : events)
        {
            const auto decodedEvent = codec.decode(event);
            if (!decodedEvent.has_value())
            {
                continue;
            }

            const auto *pressedEvent = std::get_if<KeyPressed>(&*decodedEvent);
            if (pressedEvent != nullptr && pressedEvent->key == key
                && !pressedEvent->repeat)
            {
                stopping = true;
            }
        }

        if (stopping)
        {
            events.push_back(Event{.name = kStop}); // GCOVR_EXCL_LINE
        }

        return events;
    } // GCOVR_EXCL_LINE

}
