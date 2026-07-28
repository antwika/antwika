#pragma once

#include <antwika/log/ILogger.hpp>
#include <antwika/event/IEventDispatcher.hpp>
#include <antwika/event/IEventQueue.hpp>

#include "antwika/engine/IEngine.hpp"

namespace antwika::engine
{

    using antwika::event::IEventDispatcher;
    using antwika::event::IEventQueue;
    using antwika::log::ILogger;

    /**
     * @brief IEngine implementation that logs its lifecycle and drains the
     * event queue once per tick.
     */
    class Engine : public IEngine
    {
    public:
        /**
         * @brief Construct the engine from its collaborators.
         * @param logger Receives lifecycle and per-tick log messages.
         * @param eventQueue Queue drained of pending events on every step.
         * @param dispatcher Used to broadcast the engine.tick event each step.
         */
        Engine(
            ILogger &logger,
            IEventQueue &eventQueue,
            IEventDispatcher &dispatcher);

        Engine(const Engine &) = delete;
        Engine(Engine &&) = delete;

        Engine &operator=(const Engine &) = delete;
        Engine &operator=(Engine &&) = delete;

        /**
         * @brief Log that the engine has started.
         */
        void start() override;

        /**
         * @brief Dispatch the tick event and process all queued events.
         * @param tick The tick being processed.
         */
        void step(antwika::time::Tick tick) override;

    private:
        ILogger &logger;
        IEventQueue &eventQueue;
        IEventDispatcher &dispatcher;
    };

} // namespace antwika::engine
