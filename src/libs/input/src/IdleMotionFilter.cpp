#include "antwika/input/IdleMotionFilter.hpp"

#include <utility>
#include <variant>

#include <antwika/event/ITickEventSource.hpp>

#include "antwika/input/InputEvent.hpp"

namespace antwika::input
{

    using antwika::event::ITickEventSource;

    IdleMotionFilter::IdleMotionFilter(
        ITickEventSource &innerSource, const IInputEventCodec &codec)
        : inner(innerSource), codec(codec)
    {
    }

    std::vector<Event> IdleMotionFilter::eventsFor(antwika::time::Tick tick)
    {
        auto events = inner.eventsFor(tick);

        state.beginTick();

        std::vector<Event> keptEvents;
        keptEvents.reserve(events.size() + 1);

        for (auto &event : events)
        {
            const auto decodedEvent = codec.getDecode(event);

            if (decodedEvent.has_value())
            {
                const auto *movedEvent =
                std::get_if<PointerMoved>(&*decodedEvent);
                const auto idle = !state.getMouse().isAnyDown();

                state.apply(*decodedEvent);

                if (movedEvent != nullptr && idle)
                {
                    heldMotionEvent = std::move(event);
                    continue;
                }
            }

            if (heldMotionEvent.has_value())
            {
                keptEvents.push_back(std::move(*heldMotionEvent));
                heldMotionEvent.reset();
            }

            keptEvents.push_back(std::move(event));
        }

        return keptEvents;
    }

}
