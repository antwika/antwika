#pragma once

#include <vector>

#include "Event.hpp"

namespace antwika::event
{

    class IEventHistory
    {
    public:
        virtual ~IEventHistory() = default;
        virtual std::vector<Event> getEvents() const = 0;
    };

} // namespace antwika::event
