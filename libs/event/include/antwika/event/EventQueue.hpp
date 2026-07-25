#pragma once

#include <deque>

#include "antwika/event/Event.hpp"
#include "antwika/event/IEventSink.hpp"
#include "antwika/event/IEventQueue.hpp"

namespace antwika::event
{

    class EventQueue final : public IEventQueue
    {
    public:
        void enqueue(Event event) override;
        Event pop() override;
        bool empty() const noexcept override;

    private:
        std::deque<Event> queue;
    };

} // namespace antwika::event
