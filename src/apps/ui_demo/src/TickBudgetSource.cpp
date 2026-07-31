#include "antwika/ui_demo/TickBudgetSource.hpp"

#include <antwika/engine/Events.hpp>

namespace antwika::ui_demo
{

    using antwika::engine::events::kStop;

    TickBudgetSource::TickBudgetSource(
        IReplaySource &inner, const antwika::time::Tick budget)
        : inner(inner), budget(budget)
    {
    }

    // The two excluded lines below are poker::WindowCloseSource's, for
    // its reasons: an Event carries a std::string, so appending one has
    // a throw edge and an allocating edge a short literal never takes,
    // and the closing brace is the landing pad that frees the vector on
    // an unwind nothing here can produce.
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
