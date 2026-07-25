#pragma once

#include <vector>

#include "antwika/event/Event.hpp"
#include "antwika/event/IEventHistory.hpp"
#include "antwika/event/IEventSink.hpp"

namespace antwika::event
{

    class EventRecorder : public IEventSink, IEventHistory
    {
    public:
        virtual ~EventRecorder() = default;
        virtual void handle(const Event &event);
        virtual std::vector<Event> getEvents() const;

    private:
        std::vector<Event> events;
    };

} // namespace antwika::event
