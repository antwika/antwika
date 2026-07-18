#include "antwika/engine/Engine.hpp"

#include <antwika/log/Logger.hpp>

namespace antwika::engine
{

    Engine::Engine(antwika::log::ILogger &logger) : logger(logger)
    {
    }

    void Engine::start()
    {
        logger.info("Antwika engine started!");
    }

} // namespace antwika::engine
