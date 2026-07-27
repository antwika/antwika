#pragma once

#include <vector>

#include "TimedEvent.hpp"

namespace antwika::event
{

    class ITimedEventHistory
    {
    public:
        virtual ~ITimedEventHistory() = default;
        [[nodiscard]] virtual std::vector<TimedEvent> getEvents() const = 0;
    };

} // namespace antwika::event
