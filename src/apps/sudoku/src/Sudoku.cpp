#include "antwika/sudoku/Sudoku.hpp"

#include <vector>

#include <antwika/console/ConsoleGatedSink.hpp>
#include <antwika/console/ConsoleMount.hpp>
#include <antwika/console/InputFold.hpp>
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

        BoardSink board(state, config.solveStepBudget);
        PlaySink play(
            state, overlay, config.codec, scene, config.solveStepBudget);
        StopSignal stopSignal;

        SudokuSnapshotStore snapshotStore(state);

        antwika::console::InputFold input(config.codec);

        const antwika::console::ConsoleMountSetup consoleSetup{
            .overlay = config.consoleOverlay,
            .input = input,
            .store = snapshotStore,
            .dumpPath = config.stateDumpPath,
            .loadEnabled = config.consoleLoadEnabled,
            .stop = stopSignal}; // GCOVR_EXCL_LINE
        antwika::console::ConsoleMount consoleMount(consoleSetup);

        antwika::console::ConsoleGatedSink gatedPlay =
            consoleMount.gate(play);

        std::vector<std::reference_wrapper<ITickEventSink>> timedSinks{
            input};

        if (consoleMount.mounted())
        {
            timedSinks.push_back(consoleMount.sink());
        }

        timedSinks.push_back(board);
        timedSinks.push_back(gatedPlay);
        timedSinks.push_back(stopSignal);

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

        // GCOVR_EXCL_START
        return SudokuSummary{
            .grid = state.board().format(),
            .filled = state.filled(),
            .status = state.status(),
            .commands = overlay.commands().size(),
            .console = consoleMount.state().history()};
    } // GCOVR_EXCL_STOP

}
