#pragma once

#include <cstdint>
#include <string>
#include <deque>
#include <vector>
#include <ostream>

#include "antwika/event/Event.hpp"
#include "antwika/event/IEventSink.hpp"
#include "antwika/event/IEventQueue.hpp"
#include "antwika/event/EventRecorder.hpp"

namespace antwika::event
{

    class EventQueue : public IEventQueue
    {
    public:
        explicit EventQueue(IEventSink &eventSink) noexcept;
        void enqueue(Event event) override;
        Event pop() override;
        bool empty() const noexcept override;

    private:
        std::deque<Event> queue;
        IEventSink &eventSink;
    };

} // namespace antwika::event
