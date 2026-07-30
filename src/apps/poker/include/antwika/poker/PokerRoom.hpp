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
#include <antwika/log/ILogger.hpp>
#include <antwika/replay/IReplaySource.hpp>
#include <antwika/time/IClock.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/poker/RoomConfig.hpp"
#include "antwika/poker/RoomSummary.hpp"
#include "antwika/poker/WindowSetup.hpp"

namespace antwika::poker
{

    using antwika::engine::IEngine;
    using antwika::event::IEventDispatcher;
    using antwika::event::IEventSink;
    using antwika::event::ITickEventSink;
    using antwika::log::IAppender;
    using antwika::log::IFormatter;
    using antwika::log::ILogger;
    using antwika::log::ILogPolicy;
    using antwika::replay::IReplaySource;
    using antwika::time::IClock;

    /**
     * @brief Announces the run in the log and starts the engine.
     *
     * The announcement is a log line rather than an event, because
     * nothing consumes it: as an event, every app dispatched one and then
     * stripped it by name again before writing a recording, since
     * persisting it would make a replay dispatch it twice.
     */
    class PokerRoom final
    {
    public:
        /**
         * @brief Construct the room over its engine and logger.
         * @param engine Engine started by run().
         * @param logger Receives the announcement that it is running.
         */
        explicit PokerRoom(IEngine &engine, ILogger &logger);

        PokerRoom(const PokerRoom &) = delete;
        PokerRoom(PokerRoom &&) = delete;

        PokerRoom &operator=(const PokerRoom &) = delete;
        PokerRoom &operator=(PokerRoom &&) = delete;

        /**
         * @brief Log that the run is under way and start the engine.
         */
        void run();

    private:
        IEngine &engine;
        ILogger &logger;
    };

    /**
     * @brief Everything one session is wired out of.
     *
     * A struct with designated initialisers rather than a parameter list,
     * because the list had reached eleven positional arguments, four of
     * them interchangeable-looking logging pieces and two of them raw
     * pointers a reader can only tell apart by counting.
     * A name per argument is what makes a wrong one a compile error
     * rather than a silently different session.
     */
    struct RoomSetup
    {
        /** @brief Supplies timestamps for the logger. */
        IClock &clock;

        /** @brief Receives formatted log output. */
        IAppender &appender;

        /** @brief Renders log records into text. */
        IFormatter &formatter;

        /** @brief Decides which log records are emitted. */
        ILogPolicy &logPolicy;

        /** @brief Receives every dispatched event. */
        IEventSink &eventSink;

        /** @brief Supplies each tick's events, live or replayed. */
        IReplaySource &inputSource;

        /** @brief Stream the hand-by-hand narration is written to. */
        std::ostream &out;

        /**
         * @brief How the room is set up.
         *
         * Seats, blinds, minimum buy-in and shuffle seed.
         */
        RoomConfig room = {};

        /**
         * @brief Safety cap on how many ticks to run.
         *
         * Reached without engine.stop, the session gives up rather than
         * going on forever. Production callers can leave this unset;
         * tests should always set it.
         */
        std::optional<antwika::time::Tick> maxTicks = std::nullopt;

        /**
         * @brief Sink receiving every dispatched event, stamped with its
         * tick.
         *
         * What a caller wanting to persist a `--record` file registers.
         */
        std::optional<std::reference_wrapper<ITickEventSink>>
            replayRecorder = std::nullopt;

        /**
         * @brief Graphics collaborators, which open a window.
         *
         * Set, the table is drawn into that window once per tick.
         * Closing it ends the session, and it does so through inputSource
         * like any other external input, so a windowed run and a headless
         * one reach the same result.
         */
        std::optional<std::reference_wrapper<const WindowSetup>> window =
            std::nullopt;
    };

    /**
     * @brief Wires the table, the bankrolls, the agents, the engine and
     * the replay collaborators together, then drives the tick loop until
     * an engine.stop event is dispatched.
     *
     * One tick is one step of the poker loop: the deal of a hand, or one
     * player being asked to act. Everything a session needs from outside
     * -- who deposited, who bought in for how much, who left -- arrives
     * as events from the setup's inputSource; everything else, cards and
     * decisions alike, is regenerated deterministically from the room's
     * seed and the agents' fixed policies. That is why a recorded session
     * replays to the same chip counts without a single card being stored.
     *
     * @param setup What the session is wired out of.
     * @return How the session turned out.
     * @throws antwika::gfx::GfxError If a window was asked for and could
     * not be created.
     */
    RoomSummary bootstrap(const RoomSetup &setup);

} // namespace antwika::poker
