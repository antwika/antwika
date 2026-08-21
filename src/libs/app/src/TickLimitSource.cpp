#include "antwika/app/TickLimitSource.hpp"

#include <antwika/engine/Events.hpp>
#include <antwika/event/ITickEventSource.hpp>

namespace antwika::app
{

    using antwika::engine::events::kStop;
    using antwika::event::ITickEventSource;

    TickLimitSource::TickLimitSource(
        ITickEventSource &innerSource,
        std::optional<antwika::time::Tick> limitTick)
        : inner(innerSource), limitTick(limitTick)
    {
    }

    std::vector<Event> TickLimitSource::eventsFor(
        const antwika::time::Tick tick)
    {
        auto events = inner.eventsFor(tick);

        if (limitTick.has_value() && tick >= *limitTick)
        {
            events.push_back(Event{.name = kStop}); // GCOVR_EXCL_LINE
        }

        return events;
    } // GCOVR_EXCL_LINE

}
