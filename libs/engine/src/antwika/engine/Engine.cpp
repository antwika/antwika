#include "antwika/engine/Engine.hpp"

#include <antwika/log/Logger.hpp>

namespace antwika::engine
{

    Engine::Engine(antwika::log::ILogger &logger, antwika::event::IEventQueue &eventQueue) : logger(logger), eventQueue(eventQueue)
    {
    }

    void Engine::start()
    {
        logger.log(antwika::log::Level::Info, "Antwika engine started!");

        while (!eventQueue.empty())
        {
            auto event = eventQueue.pop();
            auto message = std::format("Process event: {}", event.name); // GCOVR_EXCL_LINE
            logger.log(antwika::log::Level::Info, message);
        }
    }

} // namespace antwika::engine
