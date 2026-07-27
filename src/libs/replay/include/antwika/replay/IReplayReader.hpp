#pragma once

#include <istream>
#include <vector>

#include <antwika/event/TimedEvent.hpp>

namespace antwika::replay
{

    using antwika::event::TimedEvent;

    class IReplayReader
    {
    public:
        virtual ~IReplayReader() = default;
        [[nodiscard]] virtual std::vector<TimedEvent> read(std::istream &in) const = 0;
    };

} // namespace antwika::replay
