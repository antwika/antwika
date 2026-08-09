#include "antwika/input/CoalescingPointerSource.hpp"

#include <cstddef>

#include <antwika/event/ITickEventSource.hpp>

#include "antwika/input/Events.hpp"

namespace antwika::input
{

    using antwika::event::ITickEventSource;

    CoalescingPointerSource::CoalescingPointerSource(ITickEventSource &inner)
        : inner(inner)
    {
    }

    std::vector<Event> CoalescingPointerSource::eventsFor(
        antwika::time::Tick tick)
    {
        auto events = inner.eventsFor(tick);

        std::vector<Event> kept;
        kept.reserve(events.size());

        for (std::size_t index = 0; index < events.size(); ++index)
        {
            const auto isMove = events[index].name == events::kPointerMove;
            const auto followedByMove =
                index + 1 < events.size()
                && events[index + 1].name == events::kPointerMove;

            if (isMove && followedByMove)
            {
                continue;
            }

            kept.push_back(std::move(events[index]));
        }

        return kept;
    }

}
