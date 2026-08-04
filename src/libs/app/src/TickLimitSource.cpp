#include "antwika/app/TickLimitSource.hpp"

#include <antwika/engine/Events.hpp>
#include <antwika/event/ITickEventSource.hpp>

namespace antwika::app
{

    using antwika::engine::events::kStop;
    using antwika::event::ITickEventSource;

    TickLimitSource::TickLimitSource(
        ITickEventSource &inner, std::optional<antwika::time::Tick> limit)
        : inner(inner), limit(limit)
    {
    }

    std::vector<Event> TickLimitSource::eventsFor(
        const antwika::time::Tick tick)
    {
        auto events = inner.eventsFor(tick);

        // Appended rather than replacing what the tick carried.
        // The last thing somebody did still happens on the way out.
        // Every branch left on the excluded line is the allocator's.
        // Those are push_back's throw edge and its growth path.
        // Plus the heap branch of a name far too short to need one.
        if (limit.has_value() && tick >= *limit)
        {
            events.push_back(Event{.name = kStop}); // GCOVR_EXCL_LINE
        }

        return events;
    } // GCOVR_EXCL_LINE

} // namespace antwika::app
