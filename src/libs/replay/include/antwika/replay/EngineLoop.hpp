#pragma once

#include <optional>

#include <antwika/engine/IEngine.hpp>
#include <antwika/engine/StopSignal.hpp>
#include <antwika/event/TickedEventDispatcher.hpp>
#include <antwika/time/Tick.hpp>

#include "IReplaySource.hpp"

namespace antwika::replay
{

    using antwika::engine::IEngine;
    using antwika::engine::StopSignal;
    using antwika::event::TickedEventDispatcher;

    /**
     * @brief Drives an IEngine one fixed tick at a time until stopped.
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
         * @param dispatcher Dispatches each tick's events before stepping.
         * @param source Supplies the events to dispatch for each tick.
         */
        EngineLoop(
            IEngine &engine,
            TickedEventDispatcher &dispatcher,
            IReplaySource &source);

        EngineLoop(const EngineLoop &) = delete;
        EngineLoop(EngineLoop &&) = delete;

        EngineLoop &operator=(const EngineLoop &) = delete;
        EngineLoop &operator=(EngineLoop &&) = delete;

        /**
         * @brief Run the engine from tick 0 until stop reports stopped.
         *
         * The tick that carries the stop event still runs to completion —
         * its events are dispatched and the engine still steps — before
         * the loop exits, so live and replayed runs agree up to and
         * including the terminal tick.
         *
         * @param stop Checked after every tick; the loop exits once it
         * reports stopped().
         * @param maxTicks Optional safety cap. Production callers can
         * leave this unset to run uncapped; tests should always pass one
         * so a forgotten stop event fails loudly instead of hanging.
         * @throws EngineLoopError If maxTicks is reached without stop
         * having reported stopped().
         */
        void run(
            const StopSignal &stop,
            std::optional<antwika::time::Tick> maxTicks = std::nullopt);

    private:
        IEngine &engine;
        TickedEventDispatcher &dispatcher;
        IReplaySource &source;
    };

} // namespace antwika::replay
