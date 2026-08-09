#include "antwika/task_worker/TaskWorker.hpp"

#include <utility>

#include <antwika/console/ConsoleMount.hpp>
#include <antwika/console/InputFold.hpp>
#include <antwika/ecs/SystemScheduler.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/engine/Engine.hpp>
#include <antwika/engine/StopSignal.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/EventDispatcher.hpp>
#include <antwika/event/TickedEventDispatcher.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/simulation/EngineLoop.hpp>

#include "antwika/task_worker/JobQueue.hpp"
#include "antwika/task_worker/TaskWorkerSnapshotStore.hpp"
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

    TaskWorkerSummary bootstrap(const TaskWorkerWiring &config)
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
        JobQueue jobQueue;

        SystemScheduler systemScheduler;
        WorkerCompletionSystem completionSystem(taskRegistry);
        const auto releasePhase = systemScheduler.createPhase("release");
        systemScheduler.addSystem(releasePhase, completionSystem);

        TaskDispatchSystem dispatchSystem(
            jobQueue, lookup, taskRegistry);
        const auto dispatchPhase = systemScheduler.createPhase("dispatch");
        systemScheduler.addSystem(dispatchPhase, dispatchSystem);

        const auto observePhase = systemScheduler.createPhase("observe");
        for (auto &observer : config.observers)
        {
            systemScheduler.addSystem(observePhase, observer.get());
        }

        TaskSubmissionSink submissionSink(
            world, systemScheduler, jobQueue, lookup, taskRegistry);
        StopSignal stopSignal;

        const antwika::input::InputEventCodec codec;
        antwika::console::InputFold input(codec);

        TaskWorkerSnapshotStore snapshotStore(
            world,
            workerEntities,
            taskRegistry,
            submissionSink,
            jobQueue,
            lookup);

        const antwika::console::ConsoleMountSetup consoleSetup{
            .overlay = config.consoleOverlay,
            .input = input,
            .store = snapshotStore,
            .dumpPath = config.stateDumpPath,
            .loadEnabled = config.consoleLoadEnabled,
            .stop = stopSignal}; // GCOVR_EXCL_LINE
        antwika::console::ConsoleMount consoleMount(consoleSetup);

        std::vector<std::reference_wrapper<ITickEventSink>> timedSinks;
        if (consoleMount.mounted())
        {
            timedSinks.push_back(input);
            timedSinks.push_back(consoleMount.sink());
        }

        timedSinks.push_back(submissionSink);
        timedSinks.push_back(stopSignal);
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

        return TaskWorkerSummary{ // GCOVR_EXCL_LINE
            .workers = std::move(finalState),
            .console = consoleMount.state().history()};
    } // GCOVR_EXCL_LINE

}
