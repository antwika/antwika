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
#include <antwika/log/ILogger.hpp>
#include <antwika/simulation/ITickEventSource.hpp>
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
    using antwika::log::ILogger;
    using antwika::simulation::ITickEventSource;

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
     * @brief Everything one run of the simulation is wired out of.
     *
     * A struct with designated initialisers rather than a parameter list,
     * because the list had reached eleven positional arguments, four of
     * them interchangeable-looking logging pieces -- now one logger --
     * and two of them nullable pointers.
     * A name per argument is what makes a wrong one a compile error
     * rather than a silently different run.
     */
    struct TaskWorkerConfig
    {
        /**
         * @brief Where this run says what it is doing.
         *
         * Building it is the caller's job rather than this function's:
         * a second logger built here would be a second logger over one
         * appender, and the two would interleave their lines.
         * app::ConsoleLogging is what a composition root builds.
         */
        ILogger &logger;

        /** @brief Receives every dispatched event. */
        IEventSink &eventSink;

        /** @brief Supplies each tick's events, live or replayed. */
        ITickEventSource &inputSource;

        /** @brief Number of Worker entities to seed. */
        std::uint32_t workerCount;

        /**
         * @brief Extra systems registered into an "observe" phase.
         *
         * The phase runs after "dispatch" every tick. Empty for callers
         * that only need the final worker states.
         */
        std::vector<std::reference_wrapper<ISystem>> observers = {};

        /**
         * @brief Registry kept in sync with every task's status.
         *
         * What a caller-owned observer (e.g. StatusPrintSystem, itself
         * passed via observers) reads live during the run. Unset,
         * bootstrap() keeps one of its own, for callers with no need to
         * observe task status from outside.
         */
        std::optional<std::reference_wrapper<TaskRegistry>> registry =
            std::nullopt;

        /**
         * @brief Safety cap on how many ticks to run.
         *
         * Reached without engine.stop, the run gives up rather than going
         * on forever. Production callers can leave this unset to run
         * uncapped; tests should always set it.
         */
        std::optional<antwika::time::Tick> maxTicks = std::nullopt;

        /**
         * @brief Sink receiving every dispatched event, stamped with its
         * tick.
         *
         * What a caller wanting to persist a `--record` file registers,
         * since a run's actual length is not known ahead of time.
         */
        std::optional<std::reference_wrapper<ITickEventSink>>
            replayRecorder = std::nullopt;
    };

    /**
     * @brief Wires the ECS world, job scheduler, engine, event, and
     * replay collaborators together, boots the simulation, then drives
     * the tick loop until an engine.stop event is dispatched.
     *
     * Sources each tick's events from the config's inputSource --
     * typically events::kTaskSubmit, submitting tasks over time -- until
     * it dispatches engine.stop. A hand-scripted "live" run and a loaded
     * replay both use this same function, the same contract
     * antwika::life::bootstrap() follows for its own state.
     *
     * @param config What the run is wired out of.
     * @return Every Worker's final state, in creation order.
     */
    std::vector<Worker> bootstrap(const TaskWorkerConfig &config);

} // namespace antwika::task_worker
