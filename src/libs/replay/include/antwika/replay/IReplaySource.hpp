#pragma once

#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::replay
{

    using antwika::event::Event;

    class IReplaySource
    {
    public:
        virtual ~IReplaySource() = default;
        [[nodiscard]] virtual std::vector<Event> eventsFor(antwika::time::Tick tick) = 0;
    };

} // namespace antwika::replay
