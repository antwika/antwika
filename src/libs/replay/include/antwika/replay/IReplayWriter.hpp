#pragma once

#include <ostream>
#include <vector>

#include <antwika/event/TimedEvent.hpp>

namespace antwika::replay
{

    using antwika::event::TimedEvent;

    class IReplayWriter
    {
    public:
        virtual ~IReplayWriter() = default;
        virtual void write(const std::vector<TimedEvent> &events, std::ostream &out) const = 0;
    };

} // namespace antwika::replay
