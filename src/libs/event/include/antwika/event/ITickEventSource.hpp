#pragma once

#include <vector>

#include <antwika/time/Tick.hpp>

#include "antwika/event/Event.hpp"

namespace antwika::event
{

    class ITickEventSource
    {
    public:
        virtual ~ITickEventSource() = default;

        [[nodiscard]] virtual std::vector<Event> eventsFor(
            antwika::time::Tick tick) = 0;
    };

}
