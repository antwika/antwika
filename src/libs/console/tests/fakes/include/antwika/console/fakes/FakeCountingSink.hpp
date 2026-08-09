#pragma once

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>

namespace antwika::console::fakes
{

    struct FakeCountingSink final : antwika::event::ITickEventSink
    {
        int seen = 0;

        void handle(const antwika::event::TickEvent &) override
        {
            ++seen;
        }
    };

}
