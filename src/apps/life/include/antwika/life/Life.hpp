#pragma once

#include <cstdint>
#include <functional>
#include <vector>

#include <antwika/ecs/ISystem.hpp>
#include <antwika/engine/IEngine.hpp>
#include <antwika/event/IEventDispatcher.hpp>
#include <antwika/event/IEventQueue.hpp>
#include <antwika/event/IEventSink.hpp>
#include <antwika/log/IAppender.hpp>
#include <antwika/log/IFormatter.hpp>
#include <antwika/log/ILogPolicy.hpp>
#include <antwika/replay/IReplaySource.hpp>
#include <antwika/time/IClock.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/life/Board.hpp"

namespace antwika::life
{

    using antwika::ecs::ISystem;
    using antwika::engine::IEngine;
    using antwika::event::IEventDispatcher;
    using antwika::event::IEventQueue;
    using antwika::event::IEventSink;
    using antwika::log::IAppender;
    using antwika::log::IFormatter;
    using antwika::log::ILogPolicy;
    using antwika::replay::IReplaySource;
    using antwika::time::IClock;

    /**
     * @brief Announces simulation startup and starts the engine.
     */
    class Life
    {
    public:
        /**
         * @brief Construct the simulation over its engine and dispatcher.
         * @param engine Engine started by run().
         * @param dispatcher Dispatcher used to announce startup.
         */
        explicit Life(IEngine &engine, IEventDispatcher &dispatcher);

        Life(const Life &) = delete;
        Life(Life &&) = delete;

        Life &operator=(const Life &) = delete;
        Life &operator=(Life &&) = delete;

        /**
         * @brief Dispatch a startup event and start the engine.
         */
        void run();

    private:
        IEngine &engine;
        IEventDispatcher &dispatcher;
    };

    /**
     * @brief Wires the ECS world, engine, event, and replay collaborators
     * together, boots the simulation, then drives the fixed-timestep tick
     * loop.
     *
     * Runs for totalTicks, sourcing each tick's events from inputSource --
     * typically events::kToggleCell, seeding the initial pattern. A
     * hand-scripted "live" run and a loaded replay both use this same
     * function; they differ only in what inputSource was built from, the
     * same contract apps/game's bootstrap() follows for its own state.
     *
     * @param clock Supplies timestamps for the logger.
     * @param appender Receives formatted log output.
     * @param formatter Renders log records into text.
     * @param logPolicy Decides which log records are emitted.
     * @param eventQueue Buffers events dispatched during the run.
     * @param eventSink Receives every dispatched event.
     * @param inputSource Supplies each tick's events, live or replayed.
     * @param totalTicks The number of ticks to run.
     * @param width Number of columns in the board.
     * @param height Number of rows in the board.
     * @param observers Extra systems registered into an "observe" phase
     * that runs after "life" every tick -- each is fully independent of
     * both LifeSystem and each other (e.g. PrintSystem). Defaults to
     * none, for callers (like the tests) that only need the final Board.
     * @return The resulting Board, for callers (main.cpp, tests).
     */
    Board bootstrap(
        IClock &clock,
        IAppender &appender,
        IFormatter &formatter,
        ILogPolicy &logPolicy,
        IEventQueue &eventQueue,
        IEventSink &eventSink,
        IReplaySource &inputSource,
        antwika::time::Tick totalTicks,
        std::uint32_t width,
        std::uint32_t height,
        std::vector<std::reference_wrapper<ISystem>> observers = {});

} // namespace antwika::life
