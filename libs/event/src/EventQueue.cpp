#include "antwika/event/EventQueue.hpp"

namespace antwika::event
{

    void EventQueue::enqueue(Event event)
    {
        queue.push_back(std::move(event));
    }

    Event EventQueue::pop()
    {
        auto event = std::move(queue.front());
        queue.pop_front();
        return event;
    }

    bool EventQueue::empty() const noexcept
    {
        return queue.empty();
    }

} // namespace antwika::event
