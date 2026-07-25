#pragma once

#include <vector>

#include "antwika/event/Event.hpp"

namespace antwika::event
{
    class IEventRecorder
    {
    public:
        virtual ~IEventRecorder() = default;
        virtual void record(const Event &event) = 0;
        virtual std::vector<Event> getEvents() const = 0;
    };

} // namespace antwika::event
