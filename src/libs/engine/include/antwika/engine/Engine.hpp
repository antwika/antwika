#pragma once

#include <antwika/log/ILogger.hpp>
#include <antwika/event/IEventDispatcher.hpp>

#include "antwika/engine/IEngine.hpp"

namespace antwika::engine
{

    using antwika::event::IEventDispatcher;
    using antwika::log::ILogger;

    /**
     * @brief IEngine implementation that logs its lifecycle and broadcasts
     * the built-in tick event once per step.
     */
    class Engine : public IEngine
    {
    public:
        /**
         * @brief Construct the engine from its collaborators.
         * @param logger Receives lifecycle log messages.
         * @param dispatcher Used to broadcast the engine.tick event each step.
         */
        Engine(ILogger &logger, IEventDispatcher &dispatcher);

        Engine(const Engine &) = delete;
        Engine(Engine &&) = delete;

        Engine &operator=(const Engine &) = delete;
        Engine &operator=(Engine &&) = delete;

        /**
         * @brief Log that the engine has started.
         */
        void start() override;

        /**
         * @brief Dispatch the built-in tick event.
         * @param tick The tick being processed.
         */
        void step(antwika::time::Tick tick) override;

    private:
        ILogger &logger;
        IEventDispatcher &dispatcher;
    };

} // namespace antwika::engine
