#include "antwika/task-worker/TaskWorker.hpp"

#include <antwika/ecs/SystemScheduler.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/engine/Engine.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/EventDispatcher.hpp>
#include <antwika/event/TickedEventDispatcher.hpp>
#include <antwika/log/Logger.hpp>
#include <antwika/replay/EngineLoop.hpp>
#include <antwika/scheduler/Scheduler.hpp>

#include "antwika/task-worker/TaskDispatchSystem.hpp"
#include "antwika/task-worker/TaskSubmissionSink.hpp"
#include "antwika/task-worker/WorkerCompletionSystem.hpp"
#include "antwika/task-worker/WorkerLookup.hpp"

using antwika::ecs::Entity;
using antwika::ecs::SystemScheduler;
using antwika::ecs::World;
using antwika::engine::Engine;
using antwika::event::Event;
using antwika::event::EventDispatcher;
using antwika::event::TickedEventDispatcher;
using antwika::log::Logger;
using antwika::replay::EngineLoop;
using antwika::scheduler::Scheduler;

namespace antwika::task_worker
{

    TaskWorker::TaskWorker(IEngine &engine, IEventDispatcher &dispatcher)
        : engine(engine), dispatcher(dispatcher)
    {
    }

    void TaskWorker::run()
    {
        dispatcher.dispatch(
            Event{.name = "Running Antwika TaskWorker"}); // GCOVR_EXCL_LINE
        engine.start();
    }

    std::vector<Worker> bootstrap(
        IClock &clock,
        IAppender &appender,
        IFormatter &formatter,
        ILogPolicy &logPolicy,
        IEventSink &eventSink,
        IReplaySource &inputSource,
        antwika::time::Tick totalTicks,
        std::uint32_t workerCount,
        std::vector<std::reference_wrapper<ISystem>> observers)
    {
        Logger logger(formatter, logPolicy, clock, appender);
        EventDispatcher dispatcher({eventSink});

        World world(logger);
        std::vector<Entity> workerEntities;
        workerEntities.reserve(workerCount);
        for (std::uint32_t i = 0; i < workerCount; ++i)
        {
            const auto entity = world.create();
            world.add<Worker>(entity, Worker{});
            workerEntities.push_back(entity);
        }
        world.commit();

        WorkerLookup lookup(world, workerEntities);
        Scheduler jobScheduler;

        SystemScheduler systemScheduler;
        WorkerCompletionSystem completionSystem;
        const auto releasePhase = systemScheduler.createPhase("release");
        systemScheduler.addSystem(releasePhase, completionSystem);

        TaskDispatchSystem dispatchSystem(jobScheduler, lookup);
        const auto dispatchPhase = systemScheduler.createPhase("dispatch");
        systemScheduler.addSystem(dispatchPhase, dispatchSystem);

        const auto observePhase = systemScheduler.createPhase("observe");
        for (auto &observer : observers)
        {
            systemScheduler.addSystem(observePhase, observer.get());
        }

        TaskSubmissionSink submissionSink(
            world, systemScheduler, jobScheduler, lookup);
        TickedEventDispatcher tickedDispatcher(
            dispatcher, {submissionSink});

        Engine engine(logger, tickedDispatcher);
        TaskWorker taskWorker(engine, tickedDispatcher);
        taskWorker.run();

        EngineLoop loop(engine, tickedDispatcher, inputSource);
        loop.run(totalTicks);

        std::vector<Worker> finalState;
        finalState.reserve(workerEntities.size());
        for (const auto entity : workerEntities)
        {
            finalState.push_back(world.get<Worker>(entity));
        }
        return finalState;
    }

} // namespace antwika::task_worker
