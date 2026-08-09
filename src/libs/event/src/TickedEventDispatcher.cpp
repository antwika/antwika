#include "antwika/event/TickedEventDispatcher.hpp"

#include <utility>

#include "antwika/event/EventError.hpp"
#include "antwika/event/TickEvent.hpp"

namespace antwika::event
{

    TickedEventDispatcher::TickedEventDispatcher(
        IEventDispatcher &dispatcher,
        std::vector<std::reference_wrapper<ITickEventSink>> timedSinks)
        : dispatcher(dispatcher), timedSinks(std::move(timedSinks))
    {
    }

    void TickedEventDispatcher::setTick(antwika::time::Tick tick) noexcept
    {
        currentTick = tick;
    }

    void TickedEventDispatcher::dispatch(Event event)
    {
        if (dispatching)
        {
            throw EventError(
                "antwika::event: dispatch() re-entered from inside a "
                "sink; a derived event would be recorded against the "
                "middle of its cause");
        }

        dispatching = true;

        try
        {
            dispatcher.dispatch(event);

            TickEvent timedEvent{
                .tick = currentTick, .event = std::move(event)};

            for (auto &sink : timedSinks)
            {
                sink.get().handle(timedEvent);
            }
        }
        catch (...)
        {
            dispatching = false;
            throw;
        }

        dispatching = false;
    }

}
