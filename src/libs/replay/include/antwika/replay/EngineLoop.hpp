#pragma once

#include <antwika/engine/IEngine.hpp>
#include <antwika/event/TickedEventDispatcher.hpp>
#include <antwika/time/Tick.hpp>

#include "IReplaySource.hpp"

namespace antwika::replay
{

    using antwika::engine::IEngine;
    using antwika::event::TickedEventDispatcher;

    /**
     * @brief Drives an IEngine one fixed tick at a time.
     *
     * Every tick, it asks the IReplaySource for that tick's events,
     * dispatches those events, then steps the engine. That sequence is
     * identical for a live run or a loaded replay, which is what guarantees
     * replay reproduces the same state — deliberately, not by coincidence.
     */
    class EngineLoop
    {
    public:
        /**
         * @brief Construct the loop from its collaborators.
         * @param engine Engine stepped once per tick.
         * @param dispatcher Dispatches each tick's events before the engine steps.
         * @param source Supplies the events to dispatch for each tick.
         */
        EngineLoop(IEngine &engine, TickedEventDispatcher &dispatcher, IReplaySource &source);

        EngineLoop(const EngineLoop &) = delete;
        EngineLoop(EngineLoop &&) = delete;

        EngineLoop &operator=(const EngineLoop &) = delete;
        EngineLoop &operator=(EngineLoop &&) = delete;

        /**
         * @brief Run the engine from tick 0 up to (but excluding) totalTicks.
         * @param totalTicks The number of ticks to run.
         */
        void run(antwika::time::Tick totalTicks);

    private:
        IEngine &engine;
        TickedEventDispatcher &dispatcher;
        IReplaySource &source;
    };

} // namespace antwika::replay
