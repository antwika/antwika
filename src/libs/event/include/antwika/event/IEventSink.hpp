#pragma once

#include "antwika/event/Event.hpp"

namespace antwika::event
{

    class IEventSink
    {
    public:
        virtual ~IEventSink() = default;

        virtual void handle(const Event &event) = 0;
    };

}
