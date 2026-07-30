#include "antwika/event/EventRecorder.hpp"

namespace antwika::event
{

    void EventRecorder::handle(const Event &event)
    {
        events.push_back(event);
    }

    const std::vector<Event> &EventRecorder::getEvents() const
    {
        return events;
    }

} // namespace antwika::event
