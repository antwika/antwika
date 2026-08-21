#pragma once

#include <antwika/log/ILogger.hpp>
#include <antwika/event/IEventDispatcher.hpp>

#include "antwika/engine/IEngine.hpp"

namespace antwika::engine
{

    using antwika::event::IEventDispatcher;
    using antwika::log::ILogger;

    class Engine final : public IEngine
    {
    public:
        Engine(ILogger &logger, IEventDispatcher &dispatcher);

        Engine(const Engine &) = delete;
        Engine(Engine &&) = delete;

        Engine &operator=(const Engine &) = delete;
        Engine &operator=(Engine &&) = delete;

        void start() override;

        void step(antwika::time::Tick tick) override;

    private:
        ILogger &logger;
        IEventDispatcher &dispatcher;
    };

}
