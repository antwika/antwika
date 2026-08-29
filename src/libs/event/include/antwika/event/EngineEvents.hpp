#pragma once

#include <antwika/event/EventName.hpp>
#include <antwika/event/EventNameSeeds.hpp>

namespace antwika::event
{

    inline constexpr EventName kTick = EventName::getSeeded(kTickText);

    inline constexpr EventName kStop = EventName::getSeeded(kStopText);

}
