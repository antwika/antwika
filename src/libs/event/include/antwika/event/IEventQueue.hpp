#pragma once

#include <vector>

#include "Event.hpp"

namespace antwika::event
{

    class IEventQueue
    {
    public:
        virtual ~IEventQueue() = default;
        virtual void enqueue(Event event) = 0;
        [[nodiscard]] virtual Event pop() = 0;
        [[nodiscard]] virtual bool empty() const noexcept = 0;
    };

} // namespace antwika::event
