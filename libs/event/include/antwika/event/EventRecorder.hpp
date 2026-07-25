#pragma once

#include <vector>

#include "antwika/event/Event.hpp"
#include "antwika/event/IEventRecorder.hpp"

namespace antwika::event
{

    class EventRecorder : public IEventRecorder
    {
    public:
        virtual ~EventRecorder() = default;
        virtual void record(const Event &event);
        virtual std::vector<Event> getEvents() const;

    private:
        std::vector<Event> events;
    };

} // namespace antwika::event
