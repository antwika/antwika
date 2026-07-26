#pragma once

#include <deque>

#include "Event.hpp"
#include "IEventSink.hpp"
#include "IEventQueue.hpp"

namespace antwika::event
{

    class EventQueue final : public IEventQueue
    {
    public:
        void enqueue(Event event) override;
        [[nodiscard]] Event pop() override;
        [[nodiscard]] bool empty() const noexcept override;

    private:
        std::deque<Event> queue;
    };

} // namespace antwika::event
