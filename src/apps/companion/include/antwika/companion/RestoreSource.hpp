#pragma once

#include <optional>
#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/event/ITickEventSource.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/companion/CompanionMemory.hpp"

namespace antwika::companion
{

    using antwika::event::Event;
    using antwika::event::ITickEventSource;

    [[nodiscard]] Event restoreEvent(const CompanionMemory &memory);

    class RestoreSource final : public ITickEventSource
    {
    public:
        RestoreSource(
            ITickEventSource &inner,
            std::optional<CompanionMemory> memory);

        RestoreSource(const RestoreSource &) = delete;
        RestoreSource(RestoreSource &&) = delete;

        RestoreSource &operator=(const RestoreSource &) = delete;
        RestoreSource &operator=(RestoreSource &&) = delete;

        [[nodiscard]] std::vector<Event> eventsFor(
            antwika::time::Tick tick) override;

    private:
        ITickEventSource &inner;
        std::optional<CompanionMemory> memory;
    };

}
