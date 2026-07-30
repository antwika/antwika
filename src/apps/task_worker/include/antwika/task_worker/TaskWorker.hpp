#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

#include <antwika/ecs/ISystem.hpp>
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

#include "antwika/task_worker/TaskRegistry.hpp"
#include "antwika/task_worker/Worker.hpp"

namespace antwika::task_worker
{

    using antwika::ecs::ISystem;
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
    class TaskWorker final
    {
    public:
        /**
         * @brief Construct the simulation over its engine and logger.
         * @param engine Engine started by run().
         * @param logger Receives the announcement that it is running.
         */
        explicit TaskWorker(IEngine &engine, ILogger &logger);

        TaskWorker(const TaskWorker &) = delete;
        TaskWorker(TaskWorker &&) = delete;

        TaskWorker &operator=(const TaskWorker &) = delete;
        TaskWorker &operator=(TaskWorker &&) = delete;

        /**
         * @brief Log that the run is under way and start the engine.
         */
        void run();

    private:
        IEngine &engine;
        ILogger &logger;
    };

    /**
     * @brief Wires the ECS world, job scheduler, engine, event, and
     * replay collaborators together, boots the simulation, then drives
     * the tick loop until an engine.stop event is dispatched.
     *
     * Sources each tick's events from inputSource -- typically
     * events::kTaskSubmit, submitting tasks over time -- until it
     * dispatches engine.stop. A hand-scripted "live" run and a loaded
     * replay both use this same function, the same contract
     * antwika::life::bootstrap() follows for its own state.
     *
     * @param clock Supplies timestamps for the logger.
     * @param appender Receives formatted log output.
     * @param formatter Renders log records into text.
     * @param logPolicy Decides which log records are emitted.
     * @param eventSink Receives every dispatched event.
     * @param inputSource Supplies each tick's events, live or replayed.
     * @param workerCount Number of Worker entities to seed.
     * @param observers Extra systems registered into an "observe" phase
     * that runs after "dispatch" every tick. Defaults to none, for
     * callers (like the tests) that only need the final worker states.
     * @param registry Task registry kept in sync with every submitted
     * task's pending/completed status, for a caller-owned observer
     * (e.g. StatusPrintSystem, itself passed via observers) to read
     * live during the run. Optional: defaults to an internal registry
     * for callers with no need to observe task status externally.
     * @param maxTicks Optional safety cap on how many ticks to run before
     * giving up if engine.stop is never dispatched. Production callers
     * can leave this unset to run uncapped; tests should always pass one.
     * @param replayRecorder Optional sink that, if provided, receives
     * every dispatched event stamped with its tick -- what a caller
     * wanting to persist a `--record` file should register, since a run's
     * actual length is no longer known ahead of time. Defaults to none.
     * @return Every Worker's final state, in creation order.
     */
    std::vector<Worker> bootstrap(
        IClock &clock,
        IAppender &appender,
        IFormatter &formatter,
        ILogPolicy &logPolicy,
        IEventSink &eventSink,
        IReplaySource &inputSource,
        std::uint32_t workerCount,
        std::vector<std::reference_wrapper<ISystem>> observers = {},
        TaskRegistry *registry = nullptr,
        std::optional<antwika::time::Tick> maxTicks = std::nullopt,
        ITickEventSink *replayRecorder = nullptr);

} // namespace antwika::task_worker
