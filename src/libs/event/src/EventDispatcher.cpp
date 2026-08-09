#include "antwika/event/EventDispatcher.hpp"

#include <utility>

namespace antwika::event
{

    EventDispatcher::EventDispatcher(
        std::vector<std::reference_wrapper<IEventSink>> sinks)
        : sinks(std::move(sinks))
    {
    }

    void EventDispatcher::dispatch(Event event)
    {
        for (auto &sink : sinks)
        {
            sink.get().handle(event);
        }
    }

}
