#include "antwika/engine/Engine.hpp"

#include <format>

#include <antwika/log/Level.hpp>

using antwika::log::Level;

namespace antwika::engine
{

    Engine::Engine(ILogger &logger, IEventQueue &eventQueue) : logger(logger), eventQueue(eventQueue)
    {
    }

    void Engine::start()
    {
        logger.log(Level::Info, "Antwika engine started!");

        while (!eventQueue.empty())
        {
            auto event = eventQueue.pop();
            auto message = std::format("Process event: {}", event.name); // GCOVR_EXCL_LINE
            logger.log(Level::Info, message);
        }
    }

} // namespace antwika::engine
