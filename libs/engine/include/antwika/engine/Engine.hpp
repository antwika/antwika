#pragma once

#include <antwika/log/Logger.hpp>

#include "antwika/engine/IEngine.hpp"
#include "antwika/event/IEventQueue.hpp"

namespace antwika::engine
{

    class Engine : public IEngine
    {
    public:
        Engine(antwika::log::ILogger &logger, antwika::event::IEventQueue &eventQueue);
        void start() override;

    private:
        antwika::log::ILogger &logger;
        antwika::event::IEventQueue &eventQueue;
    };

} // namespace antwika::engine
