#pragma once

#include <cstdint>
#include <string>
#include <deque>
#include <vector>
#include <ostream>

#include "antwika/event/Event.hpp"
#include "antwika/event/IEventRecorder.hpp"
#include "antwika/event/IEventQueue.hpp"
#include "antwika/event/EventRecorder.hpp"

namespace antwika::event
{

    class EventQueue : public IEventQueue
    {
    public:
        explicit EventQueue(IEventRecorder &eventRecorder) noexcept;
        void enqueue(Event event) override;
        Event pop() override;
        bool empty() const noexcept override;
        std::vector<Event> getHistory() const override;

    private:
        std::deque<Event> queue;
        IEventRecorder &eventRecorder;
    };

} // namespace antwika::event
