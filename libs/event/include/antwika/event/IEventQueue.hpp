#pragma once

#include <vector>

#include "antwika/event/Event.hpp"

namespace antwika::event
{

    class IEventQueue
    {
    public:
        virtual ~IEventQueue() = default;
        virtual void enqueue(Event event) = 0;
        virtual Event pop() = 0;
        virtual bool empty() const noexcept = 0;
    };

} // namespace antwika::event
