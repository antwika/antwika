#pragma once

#include <vector>

#include "antwika/event/Event.hpp"
#include "antwika/event/IEventHistory.hpp"
#include "antwika/event/IEventSink.hpp"

namespace antwika::event
{

    class EventRecorder final : public IEventSink,
                                public IEventHistory
    {
    public:
        void handle(const Event &event) override;
        std::vector<Event> getEvents() const override;

    private:
        std::vector<Event> events;
    };

} // namespace antwika::event
