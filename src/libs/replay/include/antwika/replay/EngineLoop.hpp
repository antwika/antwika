#pragma once

#include <antwika/engine/IEngine.hpp>
#include <antwika/event/TickedEventDispatcher.hpp>
#include <antwika/time/Tick.hpp>

#include "IReplaySource.hpp"

namespace antwika::replay
{

    using antwika::engine::IEngine;
    using antwika::event::TickedEventDispatcher;

    // Drives IEngine one fixed tick at a time. Every tick, it first asks the
    // IReplaySource for that tick's events and dispatches them, then steps
    // the engine -- the same sequence whether the source is a hand-scripted
    // live run or a loaded replay. This is what makes "replay reproduces the
    // same state" a property of the code path itself, not a coincidence.
    class EngineLoop
    {
    public:
        EngineLoop(IEngine &engine, TickedEventDispatcher &dispatcher, IReplaySource &source);

        EngineLoop(const EngineLoop &) = delete;
        EngineLoop(EngineLoop &&) = delete;

        EngineLoop &operator=(const EngineLoop &) = delete;
        EngineLoop &operator=(EngineLoop &&) = delete;

        void run(antwika::time::Tick totalTicks);

    private:
        IEngine &engine;
        TickedEventDispatcher &dispatcher;
        IReplaySource &source;
    };

} // namespace antwika::replay
