#include "antwika/task_worker/TaskWorker.hpp"

#include <antwika/ecs/SystemScheduler.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/engine/Engine.hpp>
#include <antwika/engine/StopSignal.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/EventDispatcher.hpp>
#include <antwika/event/TickedEventDispatcher.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/simulation/EngineLoop.hpp>
#include <antwika/scheduler/Scheduler.hpp>

#include "antwika/task_worker/TaskDispatchSystem.hpp"
#include "antwika/task_worker/TaskSubmissionSink.hpp"
#include "antwika/task_worker/WorkerCompletionSystem.hpp"
#include "antwika/task_worker/WorkerLookup.hpp"

using antwika::ecs::Entity;
using antwika::ecs::SystemScheduler;
using antwika::ecs::World;
using antwika::engine::Engine;
using antwika::engine::StopSignal;
using antwika::event::Event;
using antwika::event::EventDispatcher;
using antwika::event::TickedEventDispatcher;
using antwika::log::Level;
using antwika::simulation::EngineLoop;
using antwika::scheduler::Scheduler;

namespace antwika::task_worker
{

    TaskWorker::TaskWorker(IEngine &engine, ILogger &logger)
        : engine(engine), logger(logger)
    {
    }

    void TaskWorker::run()
    {
        logger.log(Level::Info, "Running Antwika TaskWorker");
        engine.start();
    }

    std::vector<Worker> bootstrap(const TaskWorkerConfig &config)
    {
        ILogger &logger = config.logger;
        EventDispatcher dispatcher({config.eventSink});

        World world(logger);
        std::vector<Entity> workerEntities;
        workerEntities.reserve(config.workerCount);
        for (std::uint32_t i = 0; i < config.workerCount; ++i)
        {
            const auto entity = world.create();
            world.add<Worker>(entity, Worker{});
            workerEntities.push_back(entity);
        }
        world.commit();

        TaskRegistry localRegistry;
        TaskRegistry &taskRegistry = config.registry.has_value()
                                          ? config.registry->get()
                                          : localRegistry;

        WorkerLookup lookup(world, workerEntities);
        Scheduler jobScheduler;

        SystemScheduler systemScheduler;
        WorkerCompletionSystem completionSystem(taskRegistry);
        const auto releasePhase = systemScheduler.createPhase("release");
        systemScheduler.addSystem(releasePhase, completionSystem);

        TaskDispatchSystem dispatchSystem(
            jobScheduler, lookup, taskRegistry);
        const auto dispatchPhase = systemScheduler.createPhase("dispatch");
        systemScheduler.addSystem(dispatchPhase, dispatchSystem);

        const auto observePhase = systemScheduler.createPhase("observe");
        for (auto &observer : config.observers)
        {
            systemScheduler.addSystem(observePhase, observer.get());
        }

        TaskSubmissionSink submissionSink(
            world, systemScheduler, jobScheduler, lookup, taskRegistry);
        StopSignal stopSignal;

        std::vector<std::reference_wrapper<ITickEventSink>> timedSinks{
            submissionSink, stopSignal};
        if (config.replayRecorder.has_value())
        {
            timedSinks.push_back(config.replayRecorder->get());
        }
        TickedEventDispatcher tickedDispatcher(dispatcher, timedSinks);

        Engine engine(logger, tickedDispatcher);
        TaskWorker taskWorker(engine, logger);
        taskWorker.run();

        EngineLoop loop(engine, tickedDispatcher, config.inputSource);
        loop.run(stopSignal, config.maxTicks);

        std::vector<Worker> finalState;
        finalState.reserve(workerEntities.size());
        for (const auto entity : workerEntities)
        {
            finalState.push_back(world.get<Worker>(entity));
        }
        return finalState;
    }

} // namespace antwika::task_worker
