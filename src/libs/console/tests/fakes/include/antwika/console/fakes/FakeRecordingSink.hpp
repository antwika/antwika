#pragma once

#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>

namespace antwika::console::fakes
{

    struct FakeRecordingSink final : antwika::event::ITickEventSink
    {
        std::vector<antwika::event::Event> seen{};

        void handle(const antwika::event::TickEvent &event) override
        {
            seen.push_back(event.event);
        }
    };

}
