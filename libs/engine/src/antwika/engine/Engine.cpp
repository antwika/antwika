#include "antwika/engine/Engine.hpp"

#include <antwika/log/Logger.hpp>

namespace antwika::engine
{

    Engine::Engine(antwika::log::ILogger &logger, antwika::event::IEventQueue &eventQueue) : logger(logger), eventQueue(eventQueue)
    {
    }

    void Engine::start()
    {
        logger.info("Antwika engine started!");

        while (!eventQueue.empty())
        {
            auto event = eventQueue.pop();
            logger.info(std::format("Process event: {}", event.name));
        }
    }

} // namespace antwika::engine
