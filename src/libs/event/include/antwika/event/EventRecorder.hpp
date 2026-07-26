#pragma once

#include <vector>

#include "Event.hpp"
#include "IEventHistory.hpp"
#include "IEventSink.hpp"

namespace antwika::event
{

    class EventRecorder final : public IEventSink,
                                public IEventHistory
    {
    public:
        void handle(const Event &event) override;
        [[nodiscard]] std::vector<Event> getEvents() const override;

    private:
        std::vector<Event> events;
    };

} // namespace antwika::event
