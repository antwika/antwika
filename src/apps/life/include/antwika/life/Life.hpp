#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

#include <antwika/ecs/ISystem.hpp>
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

#include "antwika/life/Board.hpp"

namespace antwika::life
{

    using antwika::ecs::ISystem;
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
     * @brief Announces simulation startup and starts the engine.
     */
    class Life final
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
     * together, boots the simulation, then drives the tick loop until an
     * engine.stop event is dispatched.
     *
     * Sources each tick's events from inputSource -- typically
     * events::kToggleCell, seeding the initial pattern -- until it
     * dispatches engine.stop. A hand-scripted "live" run and a loaded
     * replay both use this same function; they differ only in what
     * inputSource was built from, the same contract apps/game's
     * bootstrap() follows for its own state.
     *
     * @param clock Supplies timestamps for the logger.
     * @param appender Receives formatted log output.
     * @param formatter Renders log records into text.
     * @param logPolicy Decides which log records are emitted.
     * @param eventSink Receives every dispatched event.
     * @param inputSource Supplies each tick's events, live or replayed.
     * @param width Number of columns in the board.
     * @param height Number of rows in the board.
     * @param observers Extra systems registered into an "observe" phase
     * that runs after "life" every tick -- each is fully independent of
     * both LifeSystem and each other (e.g. PrintSystem). Defaults to
     * none, for callers (like the tests) that only need the final Board.
     * @param maxTicks Optional safety cap on how many ticks to run before
     * giving up if engine.stop is never dispatched. Production callers
     * can leave this unset to run uncapped; tests should always pass one.
     * @param replayRecorder Optional sink that, if provided, receives
     * every dispatched event stamped with its tick -- what a caller
     * wanting to persist a `--record` file should register, since a run's
     * actual length is no longer known ahead of time. Defaults to none.
     * @return The resulting Board, for callers (main.cpp, tests).
     */
    Board bootstrap(
        IClock &clock,
        IAppender &appender,
        IFormatter &formatter,
        ILogPolicy &logPolicy,
        IEventSink &eventSink,
        IReplaySource &inputSource,
        std::uint32_t width,
        std::uint32_t height,
        std::vector<std::reference_wrapper<ISystem>> observers = {},
        std::optional<antwika::time::Tick> maxTicks = std::nullopt,
        ITimedEventSink *replayRecorder = nullptr);

} // namespace antwika::life
