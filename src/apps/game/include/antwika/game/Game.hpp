#pragma once

#include <optional>

#include <antwika/engine/IEngine.hpp>
#include <antwika/event/IEventDispatcher.hpp>
#include <antwika/event/IEventSink.hpp>
#include <antwika/event/ITimedEventSink.hpp>
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
    using antwika::event::IEventSink;
    using antwika::event::ITimedEventSink;
    using antwika::log::IAppender;
    using antwika::log::IFormatter;
    using antwika::log::ILogPolicy;
    using antwika::replay::IReplaySource;
    using antwika::time::IClock;

    /**
     * @brief Announces game startup and starts the engine.
     */
    class Game final
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
     * boots the game, then drives the tick loop until an engine.stop
     * event is dispatched.
     *
     * Sources each tick's events from inputSource until it dispatches
     * engine.stop. A hand-scripted "live" run and a loaded replay both
     * use this same function; they differ only in what inputSource was
     * built from -- and, for a replay to stop at the same tick a live run
     * did, engine.stop has to be part of that input, the same as any
     * other event.
     *
     * @param clock Supplies timestamps for the logger.
     * @param appender Receives formatted log output.
     * @param formatter Renders log records into text.
     * @param logPolicy Decides which log records are emitted.
     * @param eventSink Receives every dispatched event.
     * @param inputSource Supplies each tick's events, live or replayed.
     * @param maxTicks Optional safety cap on how many ticks to run before
     * giving up if engine.stop is never dispatched. Production callers
     * can leave this unset to run uncapped; tests should always pass one.
     * @param replayRecorder Optional sink that, if provided, receives
     * every dispatched event stamped with its tick -- what a caller
     * wanting to persist a `--record` file should register, since a run's
     * actual length is no longer known ahead of time. Defaults to none.
     * @return The resulting GameState, for callers (main.cpp, tests).
     */
    GameState bootstrap(
        IClock &clock,
        IAppender &appender,
        IFormatter &formatter,
        ILogPolicy &logPolicy,
        IEventSink &eventSink,
        IReplaySource &inputSource,
        std::optional<antwika::time::Tick> maxTicks = std::nullopt,
        ITimedEventSink *replayRecorder = nullptr);

} // namespace antwika::game
