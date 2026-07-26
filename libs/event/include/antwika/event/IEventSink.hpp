#pragma once

#include "Event.hpp"

namespace antwika::event
{

    class IEventSink
    {
    public:
        virtual ~IEventSink() = default;
        virtual void handle(const Event &event) = 0;
    };

} // namespace antwika::event
