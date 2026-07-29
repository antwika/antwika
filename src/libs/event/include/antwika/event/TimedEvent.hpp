#pragma once

#include <antwika/time/Tick.hpp>

#include "antwika/event/Event.hpp"

namespace antwika::event
{

    /**
     * @brief An Event bound to the simulation tick it occurred on.
     */
    struct TimedEvent
    {
        antwika::time::Tick tick{}; ///< Tick at which the event occurred.
        Event event{};              ///< The underlying event.
        bool operator==(const TimedEvent &other) const = default;
    };

} // namespace antwika::event
