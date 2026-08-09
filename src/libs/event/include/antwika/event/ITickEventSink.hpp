#pragma once

#include "antwika/event/TickEvent.hpp"

namespace antwika::event
{

    class ITickEventSink
    {
    public:
        virtual ~ITickEventSink() = default;

        virtual void handle(const TickEvent &event) = 0;
    };

}
