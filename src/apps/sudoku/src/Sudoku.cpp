#include "antwika/sudoku/Sudoku.hpp"

#include <vector>

#include <antwika/console/ConsoleGatedSink.hpp>
#include <antwika/console/ConsoleScene.hpp>
#include <antwika/console/ConsoleSink.hpp>
#include <antwika/console/ConsoleState.hpp>
#include <antwika/console/IConsoleControls.hpp>
#include <antwika/console/InputFold.hpp>
#include <antwika/console/SnapshotCommands.hpp>
#include <antwika/engine/Engine.hpp>
#include <antwika/engine/StopSignal.hpp>
#include <antwika/event/EventDispatcher.hpp>
#include <antwika/event/TickedEventDispatcher.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/simulation/EngineLoop.hpp>

#include "antwika/sudoku/BoardSink.hpp"
#include "antwika/sudoku/PlaySink.hpp"
#include "antwika/sudoku/SnapshotStore.hpp"
#include "antwika/sudoku/SudokuScene.hpp"

namespace antwika::sudoku
{

    using antwika::engine::Engine;
    using antwika::engine::StopSignal;
    using antwika::event::EventDispatcher;
    using antwika::event::TickedEventDispatcher;
    using antwika::simulation::EngineLoop;

    SudokuSummary bootstrap(const SudokuWiring &config)
    {
        ILogger &logger = config.logger;

        PuzzleState state;
        BoardOverlay overlay(config.canvas);
        const SudokuScene scene{config.translator};

        EventDispatcher dispatcher({config.eventSink});

        // The order is the whole wiring.
        // The scripted events land first.
        // So a solve a replay asked for is in the grid described from.
        // Then the input, which resolves this tick's clicks and keys.
        // Then whatever draws it, so a frame is of the finished tick.
        BoardSink board(state, config.solveStepBudget);
        PlaySink play(
            state, overlay, config.codec, scene, config.solveStepBudget);
        StopSignal stopSignal;

        // The console's own picture, which turns the console on.
        // Absent, no sink is registered and the state stays closed.
        // So the gate below forwards everything, untouched.
        antwika::console::ConsolePicture noConsole;
        const bool hasConsole = config.consoleOverlay.has_value();
        antwika::console::ConsolePicture &consolePicture =
            hasConsole ? config.consoleOverlay->get() : noConsole;

        antwika::console::ConsoleState console;
        const antwika::console::ConsoleScene consoleScene;
        const antwika::console::FixedConsoleControls consoleControls{};

        // This application's half of the console's snapshot seam.
        // The whole session is the one PuzzleState.
        SudokuSnapshotStore snapshotStore(state);
        antwika::console::SnapshotCommands consoleCommands(
            snapshotStore,
            config.stateDumpPath,
            config.consoleLoadEnabled);

        antwika::console::InputFold input(config.codec);
        antwika::console::ConsoleSink consoleSink(
            antwika::console::ConsoleSinkSetup{
                .console = console,
                .input = input,
                .picture = consolePicture,
                .scene = consoleScene,
                .controls = consoleControls,
                .commands = consoleCommands});

        // The console is on top, so what it stands over it takes.
        // PlaySink is the one sink that reads a key or a pixel.
        // Fully open, a digit types into the field, never a square.
        antwika::console::ConsoleGatedSink gatedPlay(
            play, console, input);

        // The fold is first, ahead of everything that reads it.
        // ConsoleSink is straight after, ahead of what it gates.
        // A press has to be the console's before the board may ask.
        std::vector<std::reference_wrapper<ITickEventSink>> timedSinks{
            input};

        // Registered only when there is somewhere to put the picture.
        // "No console" then means no console, not an invisible one.
        if (hasConsole)
        {
            timedSinks.push_back(consoleSink);
        }

        timedSinks.push_back(board);
        timedSinks.push_back(gatedPlay);
        timedSinks.push_back(stopSignal);

        // Held out here rather than inside the if.
        // The sink has to outlive the reference the dispatcher keeps.
        std::unique_ptr<ITickEventSink> extra;
        if (config.extraSink)
        {
            extra = config.extraSink(state, overlay);
            timedSinks.push_back(*extra);
        }

        if (config.replayRecorder.has_value())
        {
            timedSinks.push_back(config.replayRecorder->get());
        }

        TickedEventDispatcher tickedDispatcher(dispatcher, timedSinks);
        Engine engine(logger, tickedDispatcher);

        logger.log(antwika::log::Level::Info, "Running antwika sudoku");
        engine.start();

        EngineLoop loop(engine, tickedDispatcher, config.inputSource);
        loop.run(stopSignal, config.maxTicks);

        // The excluded lines below are the allocator's alone.
        // Returning a summary that owns a string is what makes them.
        // And gcov puts the cleanup block on the closing brace.
        // See docs/confirming-unreachable-branches.md.
        // GCOVR_EXCL_START
        return SudokuSummary{
            .grid = state.board().format(),
            .filled = state.filled(),
            .status = state.status(),
            .commands = overlay.commands().size(),
            .console = console.history()};
    } // GCOVR_EXCL_STOP

} // namespace antwika::sudoku
