#include "antwika/event/ReplayRecorder.hpp"

namespace antwika::event
{

    void ReplayRecorder::handle(const TimedEvent &event)
    {
        events.push_back(event);
    }

    std::vector<TimedEvent> ReplayRecorder::getEvents() const
    {
        return events;
    }

} // namespace antwika::event
