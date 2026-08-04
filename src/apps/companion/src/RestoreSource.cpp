#include "antwika/companion/RestoreSource.hpp"
#include <antwika/event/ITickEventSource.hpp>

#include <utility>

#include <nlohmann/json.hpp>

#include "antwika/companion/Events.hpp"
#include "antwika/companion/PetSave.hpp"

namespace antwika::companion
{

    using antwika::event::ITickEventSource;

    Event restoreEvent(const CompanionMemory &memory)
    {
        // The saved document itself, not a second encoding of one.
        // So a file an older build wrote is migrated on its way in here.
        // Exactly as it is on its way in from disk.
        // And the two cannot describe one companion differently.
        //
        // The excluded lines below are the allocator's alone.
        // Building an Event that owns two strings is what makes them.
        // And gcov puts the cleanup block on the closing brace.
        // See docs/confirming-unreachable-branches.md.
        // GCOVR_EXCL_START
        return Event{
            .name = events::kRestore,
            .payload = companionMemoryToJson(memory).dump()};
    } // GCOVR_EXCL_STOP

    RestoreSource::RestoreSource(
        ITickEventSource &inner, std::optional<CompanionMemory> memory)
        : inner(inner), memory(std::move(memory))
    {
    }

    std::vector<Event> RestoreSource::eventsFor(
        const antwika::time::Tick tick)
    {
        auto events = inner.eventsFor(tick);

        if (memory.has_value())
        {
            // Every branch on the excluded lines is the allocator's.
            // Those are insert's throw edge and its growth path.
            auto announcement = restoreEvent(*memory);
            // GCOVR_EXCL_START
            events.insert(events.begin(), std::move(announcement));
            // GCOVR_EXCL_STOP
            memory.reset();
        }

        return events;
    } // GCOVR_EXCL_LINE

} // namespace antwika::companion
