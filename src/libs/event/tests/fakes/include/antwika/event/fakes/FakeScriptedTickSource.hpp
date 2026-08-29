#pragma once

#include <utility>
#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/event/ITickEventSource.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::event::fakes
{

    class FakeScriptedTickSource final : public ITickEventSource
    {
    public:
        [[nodiscard]] std::vector<Event> eventsFor(
            antwika::time::Tick tick) override
        {
            static_cast<void>(tick);

            return std::exchange(nextEvents, {});
        }

        std::vector<Event> nextEvents;
    };

}
