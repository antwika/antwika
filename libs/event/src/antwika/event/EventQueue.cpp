#include "antwika/event/EventQueue.hpp"

namespace antwika::event
{

    EventQueue::EventQueue(IEventRecorder &eventRecorder) noexcept : eventRecorder(eventRecorder) { // GCOVR_EXCL_LINE

    }

    void EventQueue::enqueue(Event event)
    {
        eventRecorder.record(event);
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

    std::vector<Event> EventQueue::getHistory() const
    {
        return eventRecorder.getEvents();
    }

} // namespace antwika::event
