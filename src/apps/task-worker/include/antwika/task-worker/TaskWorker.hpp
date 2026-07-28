#pragma once

#include <cstdint>
#include <functional>
#include <vector>

#include <antwika/ecs/ISystem.hpp>
#include <antwika/engine/IEngine.hpp>
#include <antwika/event/IEventDispatcher.hpp>
#include <antwika/event/IEventSink.hpp>
#include <antwika/log/IAppender.hpp>
#include <antwika/log/IFormatter.hpp>
#include <antwika/log/ILogPolicy.hpp>
#include <antwika/replay/IReplaySource.hpp>
#include <antwika/time/IClock.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/task-worker/Worker.hpp"

namespace antwika::task_worker
{

    using antwika::ecs::ISystem;
    using antwika::engine::IEngine;
    using antwika::event::IEventDispatcher;
    using antwika::event::IEventSink;
    using antwika::log::IAppender;
    using antwika::log::IFormatter;
    using antwika::log::ILogPolicy;
    using antwika::replay::IReplaySource;
    using antwika::time::IClock;

    /**
     * @brief Announces simulation startup and starts the engine.
     */
    class TaskWorker
    {
    public:
        /**
         * @brief Construct the simulation over its engine and dispatcher.
         * @param engine Engine started by run().
         * @param dispatcher Dispatcher used to announce startup.
         */
        explicit TaskWorker(IEngine &engine, IEventDispatcher &dispatcher);

        TaskWorker(const TaskWorker &) = delete;
        TaskWorker(TaskWorker &&) = delete;

        TaskWorker &operator=(const TaskWorker &) = delete;
        TaskWorker &operator=(TaskWorker &&) = delete;

        /**
         * @brief Dispatch a startup event and start the engine.
         */
        void run();

    private:
        IEngine &engine;
        IEventDispatcher &dispatcher;
    };

    /**
     * @brief Wires the ECS world, job scheduler, engine, event, and
     * replay collaborators together, boots the simulation, then drives
     * the fixed-timestep tick loop.
     *
     * Runs for totalTicks, sourcing each tick's events from
     * inputSource -- typically events::kTaskSubmit, submitting tasks
     * over time. A hand-scripted "live" run and a loaded replay both
     * use this same function, the same contract
     * antwika::life::bootstrap() follows for its own state.
     *
     * @param clock Supplies timestamps for the logger.
     * @param appender Receives formatted log output.
     * @param formatter Renders log records into text.
     * @param logPolicy Decides which log records are emitted.
     * @param eventSink Receives every dispatched event.
     * @param inputSource Supplies each tick's events, live or replayed.
     * @param totalTicks The number of ticks to run.
     * @param workerCount Number of Worker entities to seed.
     * @param observers Extra systems registered into an "observe" phase
     * that runs after "dispatch" every tick. Defaults to none, for
     * callers (like the tests) that only need the final worker states.
     * @return Every Worker's final state, in creation order.
     */
    std::vector<Worker> bootstrap(
        IClock &clock,
        IAppender &appender,
        IFormatter &formatter,
        ILogPolicy &logPolicy,
        IEventSink &eventSink,
        IReplaySource &inputSource,
        antwika::time::Tick totalTicks,
        std::uint32_t workerCount,
        std::vector<std::reference_wrapper<ISystem>> observers = {});

} // namespace antwika::task_worker
