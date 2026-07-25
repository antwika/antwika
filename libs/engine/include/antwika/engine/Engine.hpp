#pragma once

#include <antwika/log/Logger.hpp>

#include "antwika/event/IEventQueue.hpp"

namespace antwika::engine
{

    class Engine
    {
    public:
        Engine(antwika::log::ILogger &logger, antwika::event::IEventQueue &eventQueue);

        void start();

    private:
        antwika::log::ILogger &logger;
        antwika::event::IEventQueue &eventQueue;
    };

} // namespace antwika::engine
