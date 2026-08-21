#pragma once

#include <cstddef>
#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/event/ITickEventSource.hpp>

namespace antwika::replay
{

    using antwika::event::Event;
    using antwika::event::TickEvent;
    using antwika::event::ITickEventSource;

    class ReplaySource final : public ITickEventSource
    {
    public:
        explicit ReplaySource(std::vector<TickEvent> events);

        [[nodiscard]] std::vector<Event> eventsFor(
            antwika::time::Tick tick) override;

    private:
        std::vector<TickEvent> events;

        std::size_t cursor = 0;
    };

}
