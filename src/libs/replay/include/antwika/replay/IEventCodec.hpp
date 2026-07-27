#pragma once

#include <istream>
#include <ostream>

#include <antwika/event/TimedEvent.hpp>

namespace antwika::replay
{

    using antwika::event::TimedEvent;

    class IEventCodec
    {
    public:
        virtual ~IEventCodec() = default;
        virtual void encode(const TimedEvent &event, std::ostream &out) const = 0;
        [[nodiscard]] virtual TimedEvent decode(std::istream &in) const = 0;
    };

} // namespace antwika::replay
