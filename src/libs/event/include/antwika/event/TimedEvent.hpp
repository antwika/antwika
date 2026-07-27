#pragma once

#include <antwika/time/Tick.hpp>

#include "Event.hpp"

namespace antwika::event
{

    struct TimedEvent
    {
        antwika::time::Tick tick{};
        Event event{};
        bool operator==(const TimedEvent &other) const = default;
    };

} // namespace antwika::event
