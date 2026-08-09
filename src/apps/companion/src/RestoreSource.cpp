#include "antwika/companion/RestoreSource.hpp"

#include <nlohmann/json.hpp>

#include <utility>

#include <antwika/event/ITickEventSource.hpp>

#include "antwika/companion/Events.hpp"
#include "antwika/companion/PetSave.hpp"

namespace antwika::companion
{

    using antwika::event::ITickEventSource;

    Event restoreEvent(const CompanionMemory &memory)
    {
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
            auto announcement = restoreEvent(*memory);
            // GCOVR_EXCL_START
            events.insert(events.begin(), std::move(announcement));
            // GCOVR_EXCL_STOP
            memory.reset();
        }

        return events;
    } // GCOVR_EXCL_LINE

}
