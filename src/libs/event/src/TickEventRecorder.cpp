#include "antwika/event/TickEventRecorder.hpp"

namespace antwika::event
{

    void TickEventRecorder::handle(const TickEvent &event)
    {
        events.push_back(event);
    }

    const std::vector<TickEvent> &TickEventRecorder::getEvents() const
    {
        return events;
    }

} // namespace antwika::event
