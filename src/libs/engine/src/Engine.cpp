#include "antwika/engine/Engine.hpp"

#include <format>

#include <antwika/event/Event.hpp>
#include <antwika/log/Level.hpp>

#include "antwika/engine/Events.hpp"

using antwika::event::Event;
using antwika::log::Level;

namespace antwika::engine
{

    Engine::Engine(ILogger &logger, IEventQueue &eventQueue, IEventDispatcher &dispatcher) : logger(logger), eventQueue(eventQueue), dispatcher(dispatcher)
    {
    }

    void Engine::start()
    {
        logger.log(Level::Info, "Antwika engine started!");
    }

    void Engine::step(antwika::time::Tick tick)
    {
        auto stepMessage = std::format("Engine step: tick {}", tick); // GCOVR_EXCL_LINE
        logger.log(Level::Info, stepMessage);

        dispatcher.dispatch(Event{.name = events::kTick}); // GCOVR_EXCL_LINE

        while (!eventQueue.empty())
        {
            auto event = eventQueue.pop();
            auto message = std::format("Process event: {}", event.name); // GCOVR_EXCL_LINE
            logger.log(Level::Info, message);
        }
    }

} // namespace antwika::engine
