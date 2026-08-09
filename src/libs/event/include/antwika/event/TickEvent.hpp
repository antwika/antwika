#pragma once

#include <antwika/time/Tick.hpp>

#include "antwika/event/Event.hpp"

namespace antwika::event
{

    struct TickEvent final
    {
        antwika::time::Tick tick{};
        Event event{};
        bool operator==(const TickEvent &other) const = default;
    };

}
