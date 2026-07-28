#pragma once

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

#include "antwika/game/GameState.hpp"

namespace antwika::game
{

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
     * @brief Announces game startup and starts the engine.
     */
    class Game
    {
    public:
        /**
         * @brief Construct the game over its engine and event dispatcher.
         * @param engine Engine started by run().
         * @param dispatcher Dispatcher used to announce the game is running.
         */
        explicit Game(IEngine &engine, IEventDispatcher &dispatcher);

        Game(const Game &) = delete;
        Game(Game &&) = delete;

        Game &operator=(const Game &) = delete;
        Game &operator=(Game &&) = delete;

        /**
         * @brief Dispatch a startup event and start the engine.
         */
        void run();

    private:
        IEngine &engine;
        IEventDispatcher &dispatcher;
    };

    /**
     * @brief Wires the engine, event, and replay collaborators together,
     * boots the game, then drives the fixed-timestep tick loop.
     *
     * Runs for totalTicks, sourcing each tick's events from inputSource. A
     * hand-scripted "live" run and a loaded replay both use this same
     * function; they differ only in what inputSource was built from.
     *
     * @param clock Supplies timestamps for the logger.
     * @param appender Receives formatted log output.
     * @param formatter Renders log records into text.
     * @param logPolicy Decides which log records are emitted.
     * @param eventQueue Buffers events dispatched during the run.
     * @param eventSink Receives every dispatched event.
     * @param inputSource Supplies each tick's events, live or replayed.
     * @param totalTicks The number of ticks to run.
     * @return The resulting GameState, for callers (main.cpp, tests).
     */
    GameState bootstrap(
        IClock &clock,
        IAppender &appender,
        IFormatter &formatter,
        ILogPolicy &logPolicy,
        IEventQueue &eventQueue,
        IEventSink &eventSink,
        IReplaySource &inputSource,
        antwika::time::Tick totalTicks);

} // namespace antwika::game
