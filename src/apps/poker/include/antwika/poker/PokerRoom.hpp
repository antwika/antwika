#pragma once

#include <optional>
#include <ostream>

#include <antwika/engine/IEngine.hpp>
#include <antwika/event/IEventDispatcher.hpp>
#include <antwika/event/IEventSink.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/log/IAppender.hpp>
#include <antwika/log/IFormatter.hpp>
#include <antwika/log/ILogPolicy.hpp>
#include <antwika/replay/IReplaySource.hpp>
#include <antwika/time/IClock.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/poker/RoomConfig.hpp"
#include "antwika/poker/RoomSummary.hpp"

namespace antwika::poker
{

    using antwika::engine::IEngine;
    using antwika::event::IEventDispatcher;
    using antwika::event::IEventSink;
    using antwika::event::ITickEventSink;
    using antwika::log::IAppender;
    using antwika::log::IFormatter;
    using antwika::log::ILogPolicy;
    using antwika::replay::IReplaySource;
    using antwika::time::IClock;

    /**
     * @brief Announces the room opening and starts the engine.
     */
    class PokerRoom final
    {
    public:
        /**
         * @brief Construct the room over its engine and dispatcher.
         * @param engine Engine started by run().
         * @param dispatcher Dispatcher used to announce startup.
         */
        explicit PokerRoom(IEngine &engine, IEventDispatcher &dispatcher);

        PokerRoom(const PokerRoom &) = delete;
        PokerRoom(PokerRoom &&) = delete;

        PokerRoom &operator=(const PokerRoom &) = delete;
        PokerRoom &operator=(PokerRoom &&) = delete;

        /**
         * @brief Dispatch a startup event and start the engine.
         */
        void run();

    private:
        IEngine &engine;
        IEventDispatcher &dispatcher;
    };

    /**
     * @brief Wires the table, the bankrolls, the agents, the engine and
     * the replay collaborators together, then drives the tick loop until
     * an engine.stop event is dispatched.
     *
     * One tick is one step of the poker loop: the deal of a hand, or one
     * player being asked to act. Everything a session needs from outside
     * -- who deposited, who bought in for how much, who left -- arrives
     * as events from inputSource; everything else, cards and decisions
     * alike, is regenerated deterministically from config.seed and the
     * agents' fixed policies. That is why a recorded session replays to
     * the same chip counts without a single card being stored.
     *
     * @param clock Supplies timestamps for the logger.
     * @param appender Receives formatted log output.
     * @param formatter Renders log records into text.
     * @param logPolicy Decides which log records are emitted.
     * @param eventSink Receives every dispatched event.
     * @param inputSource Supplies each tick's events, live or replayed.
     * @param out Stream the hand-by-hand narration is written to.
     * @param config How the room is set up: seats, blinds, minimum
     * buy-in and shuffle seed.
     * @param maxTicks Optional safety cap on how many ticks to run
     * before giving up if engine.stop is never dispatched. Production
     * callers can leave this unset; tests should always pass one.
     * @param replayRecorder Optional sink that, if provided, receives
     * every dispatched event stamped with its tick -- what a caller
     * wanting to persist a `--record` file should register.
     * @return How the session turned out.
     */
    RoomSummary bootstrap(
        IClock &clock,
        IAppender &appender,
        IFormatter &formatter,
        ILogPolicy &logPolicy,
        IEventSink &eventSink,
        IReplaySource &inputSource,
        std::ostream &out,
        RoomConfig config = {},
        std::optional<antwika::time::Tick> maxTicks = std::nullopt,
        ITickEventSink *replayRecorder = nullptr);

} // namespace antwika::poker
