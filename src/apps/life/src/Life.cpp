#include "antwika/life/Life.hpp"

#include <memory>
#include <optional>

#include <antwika/console/ConsoleGatedSink.hpp>
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

#include "antwika/life/BoardSink.hpp"
#include "antwika/life/DragPausedSystem.hpp"
#include "antwika/life/Events.hpp"
#include "antwika/life/Grid.hpp"
#include "antwika/life/LifeSnapshotStore.hpp"
#include "antwika/life/LifeSystem.hpp"

using antwika::ecs::SystemScheduler;
using antwika::ecs::World;
using antwika::engine::Engine;
using antwika::engine::StopSignal;
using antwika::event::Event;
using antwika::event::EventDispatcher;
using antwika::event::TickedEventDispatcher;
using antwika::log::Level;
using antwika::simulation::EngineLoop;

namespace antwika::life
{

    Life::Life(IEngine &engine, ILogger &logger)
        : engine(engine), logger(logger)
    {
    }

    void Life::run()
    {
        logger.log(Level::Info, "Running Antwika Life");
        engine.start();
    }

    LifeSummary bootstrap(const LifeWiring &config)
    {
        ILogger &logger = config.logger;

        EventDispatcher dispatcher({config.eventSink});

        World world(logger);
        Grid grid(world, config.width, config.height);
        world.commit();

        SystemScheduler scheduler;
        LifeSystem lifeSystem(grid);

        DragState drag;
        DragPausedSystem pausedLife(lifeSystem, drag);

        const auto lifePhase = scheduler.createPhase("life");
        scheduler.addSystem(lifePhase, pausedLife);

        const auto observePhase = scheduler.createPhase("observe");
        for (auto &observer : config.observers)
        {
            scheduler.addSystem(observePhase, observer.get());
        }

        BoardSink boardSink(world, grid, scheduler);
        StopSignal stopSignal;

        std::unique_ptr<PointerToggleSink> extra;
        std::optional<std::reference_wrapper<PointerToggleSink>> pointer;
        if (config.extraSink)
        {
            extra = config.extraSink(world, grid, drag);
            pointer = *extra;
        }

        const antwika::input::InputEventCodec consoleCodec;
        antwika::console::InputFold input(consoleCodec);

        LifeSnapshotStore snapshotStore(world, grid, drag, pointer);

        const antwika::console::ConsoleMountSetup consoleSetup{
            .overlay = config.consoleOverlay,
            .input = input,
            .store = snapshotStore,
            .dumpPath = config.stateDumpPath,
            .loadEnabled = config.consoleLoadEnabled,
            .stop = stopSignal}; // GCOVR_EXCL_LINE
        antwika::console::ConsoleMount consoleMount(consoleSetup);

        std::optional<antwika::console::ConsoleGatedSink> gatedPointer;
        if (extra)
        {
            gatedPointer.emplace(
                *extra,
                consoleMount.state(),
                input,
                consoleMount.events());
        }

        std::vector<std::reference_wrapper<ITickEventSink>> timedSinks{
            input};

        if (consoleMount.mounted())
        {
            timedSinks.push_back(consoleMount.sink());
        }

        timedSinks.push_back(boardSink);
        timedSinks.push_back(stopSignal);

        if (gatedPointer.has_value())
        {
            timedSinks.push_back(*gatedPointer);
        }

        if (config.replayRecorder.has_value())
        {
            timedSinks.push_back(config.replayRecorder->get());
        }
        TickedEventDispatcher tickedDispatcher(dispatcher, timedSinks);

        Engine engine(logger, tickedDispatcher);
        Life life(engine, logger);
        life.run();

        EngineLoop loop(engine, tickedDispatcher, config.inputSource);
        loop.run(stopSignal, config.maxTicks);

        return LifeSummary{ // GCOVR_EXCL_LINE
            .board = readBoard(world, grid),
            .console = consoleMount.state().history()};
    } // GCOVR_EXCL_LINE

    std::vector<std::reference_wrapper<ISystem>> observersFor(
        ISystem &renderer,
        ISystem &printer,
        ISystem &pacer,
        bool drawsNothing)
    {
        if (drawsNothing)
        {
            return {renderer, printer, pacer};
        }

        return {renderer, pacer};
    }

    void announceHowToStop(ILogger &logger, bool drawsNothing)
    {
        if (!drawsNothing)
        {
            return;
        }

        logger.log(
            antwika::log::Level::Info,
            "Antwika Life: this backend has no window to close, so "
            "press Ctrl+C to stop");
    }

}
