#include "antwika/input/CoalescingPointerSource.hpp"

#include <cstddef>

#include "antwika/input/Events.hpp"

namespace antwika::input
{

    CoalescingPointerSource::CoalescingPointerSource(IReplaySource &inner)
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

            // A movement superseded inside the tick was never read.
            if (isMove && followedByMove)
            {
                continue;
            }

            kept.push_back(std::move(events[index]));
        }

        return kept;
    }

} // namespace antwika::input
