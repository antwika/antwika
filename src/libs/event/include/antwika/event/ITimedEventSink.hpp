#pragma once

#include "TimedEvent.hpp"

namespace antwika::event
{

    class ITimedEventSink
    {
    public:
        virtual ~ITimedEventSink() = default;
        virtual void handle(const TimedEvent &event) = 0;
    };

} // namespace antwika::event
