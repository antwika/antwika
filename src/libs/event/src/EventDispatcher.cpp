#include "antwika/event/EventDispatcher.hpp"

#include <utility>

namespace antwika::event
{

    EventDispatcher::EventDispatcher(
        IEventQueue &queue,
        std::vector<std::reference_wrapper<IEventSink>> sinks)
        : queue(queue), sinks(std::move(sinks))
    {
    }

    void EventDispatcher::dispatch(Event event)
    {
        for (auto &sink : sinks)
        {
            sink.get().handle(event);
        }

        queue.enqueue(std::move(event));
    }

} // namespace antwika::event
