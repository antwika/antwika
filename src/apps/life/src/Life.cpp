#include "antwika/life/Life.hpp"

#include <memory>
#include <optional>

#include <antwika/console/ConsoleGatedSink.hpp>
#include <antwika/console/ConsoleScene.hpp>
#include <antwika/console/ConsoleSink.hpp>
#include <antwika/console/ConsoleState.hpp>
#include <antwika/console/IConsoleControls.hpp>
#include <antwika/console/InputFold.hpp>
#include <antwika/console/SnapshotCommands.hpp>
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

        // A board being drawn on stands still.
        // A cell toggled on one tick is then still there on the next.
        // Only a sink reporting a drag can ever start one.
        // A run that registered none is therefore unaffected.
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

        // Held out here rather than inside the if.
        // The sink has to outlive the reference the dispatcher keeps.
        std::unique_ptr<PointerToggleSink> extra;
        std::optional<std::reference_wrapper<PointerToggleSink>> pointer;
        if (config.extraSink)
        {
            extra = config.extraSink(world, grid, drag);
            pointer = *extra;
        }

        // The console's own picture, which turns the console on.
        // Absent, no ConsoleSink is registered at all.
        // The state then stays closed for the whole run.
        // So the gate below forwards everything, untouched.
        antwika::console::ConsolePicture noConsole;
        const bool hasConsole = config.consoleOverlay.has_value();
        antwika::console::ConsolePicture &consolePicture =
            hasConsole ? config.consoleOverlay->get() : noConsole;

        antwika::console::ConsoleState console;
        const antwika::console::ConsoleScene consoleScene;

        // The shipped constants: Grave, Enter, the Swedish board.
        // This application has no options screen to rebind them on.
        const antwika::console::FixedConsoleControls consoleControls;

        // The fold reads the standard wire format every run writes.
        // Owned here rather than injected, since only the console reads it.
        // The pointer sink keeps the caller's own codec.
        const antwika::input::InputEventCodec consoleCodec;
        antwika::console::InputFold input(consoleCodec);

        LifeSnapshotStore snapshotStore(world, grid, drag, pointer);
        antwika::console::SnapshotCommands consoleCommands(
            snapshotStore,
            config.stateDumpPath,
            config.consoleLoadEnabled);

        antwika::console::ConsoleSink consoleSink(
            antwika::console::ConsoleSinkSetup{
                .console = console,
                .input = input,
                .picture = consolePicture,
                .scene = consoleScene,
                .controls = consoleControls,
                .commands = consoleCommands});

        // The console is on top, so what it stands over it takes.
        // Only the pointer sink reads a key or a pixel here.
        // BoardSink folds this app's own scripted events.
        // StopSignal folds engine.stop.
        // Neither reads input, so neither is gated.
        std::optional<antwika::console::ConsoleGatedSink> gatedPointer;
        if (extra)
        {
            gatedPointer.emplace(*extra, console, input);
        }

        // The fold is first.
        // What it holds is the event the sinks after it are given now.
        // ConsoleSink is ahead of everything it gates.
        // A press has to be the console's before the board may ask.
        std::vector<std::reference_wrapper<ITickEventSink>> timedSinks{
            input};

        // Registered only when there is somewhere to put the picture.
        // "No console" then means no console, not an invisible one.
        if (hasConsole)
        {
            timedSinks.push_back(consoleSink);
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

        // Every branch left on the excluded line is the allocator's.
        // The throw edges of copying the board and the history.
        return LifeSummary{ // GCOVR_EXCL_LINE
            .board = readBoard(world, grid),
            .console = console.history()};
        // The excluded line is the local summary's unwind destructor.
        // Nothing between its construction and the return throws.
    } // GCOVR_EXCL_LINE

    std::vector<std::reference_wrapper<ISystem>> observersFor(
        ISystem &renderer,
        ISystem &printer,
        ISystem &pacer,
        bool drawsNothing)
    {
        // Two whole lists rather than one built up in place.
        // A named vector leaves an unwind cleanup line nothing reaches.
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

} // namespace antwika::life
