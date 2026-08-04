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
        ITickEventSource &inner, const IInputEventCodec &codec, Key key)
        : inner(inner), codec(codec), key(key)
    {
    }

    std::vector<Event> StopOnKeySource::eventsFor(antwika::time::Tick tick)
    {
        auto events = inner.eventsFor(tick);

        bool stopping = false;
        for (const auto &event : events)
        {
            const auto decoded = codec.decode(event);
            if (!decoded.has_value())
            {
                continue;
            }

            const auto *pressed = std::get_if<KeyPressed>(&*decoded);
            if (pressed != nullptr && pressed->key == key
                && !pressed->repeat)
            {
                stopping = true;
            }
        }

        // One stop, however many times the key was pressed this tick.
        if (stopping)
        {
            events.push_back(Event{.name = kStop}); // GCOVR_EXCL_LINE
        }

        return events;
    } // GCOVR_EXCL_LINE

} // namespace antwika::input
