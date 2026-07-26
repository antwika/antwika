#pragma once

#include <antwika/log/ILogger.hpp>
#include <antwika/event/IEventQueue.hpp>

#include "antwika/engine/IEngine.hpp"

namespace antwika::engine
{

    using antwika::event::IEventQueue;
    using antwika::log::ILogger;

    class Engine : public IEngine
    {
    public:
        Engine(ILogger &logger, IEventQueue &eventQueue);
        void start() override;

    private:
        ILogger &logger;
        IEventQueue &eventQueue;
    };

} // namespace antwika::engine
