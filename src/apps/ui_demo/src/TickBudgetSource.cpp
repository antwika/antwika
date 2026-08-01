#include "antwika/ui_demo/TickBudgetSource.hpp"

#include <antwika/engine/Events.hpp>

namespace antwika::ui_demo
{

    using antwika::engine::events::kStop;

    TickBudgetSource::TickBudgetSource(
        ITickEventSource &inner, const antwika::time::Tick budget)
        : inner(inner), budget(budget)
    {
    }

    // The two excluded lines below are poker::WindowCloseSource's.
    // Appending an Event has a throw edge and an allocating edge.
    // A short literal never takes either.
    // The closing brace is an unwind landing pad nothing here reaches.
    // See docs/confirming-unreachable-branches.md.
    std::vector<Event> TickBudgetSource::eventsFor(
        const antwika::time::Tick tick)
    {
        auto events = inner.eventsFor(tick);

        // No guard against saying this twice.
        // StopSignal ends the loop on the tick carrying the first one.
        // And a second engine.stop would change nothing anyway.
        if (tick >= budget)
        {
            events.push_back(Event{.name = kStop}); // GCOVR_EXCL_LINE
        }

        return events;
    } // GCOVR_EXCL_LINE

} // namespace antwika::ui_demo
