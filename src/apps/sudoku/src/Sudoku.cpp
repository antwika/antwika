#include "antwika/sudoku/Sudoku.hpp"

#include <vector>

#include <antwika/engine/Engine.hpp>
#include <antwika/engine/StopSignal.hpp>
#include <antwika/event/EventDispatcher.hpp>
#include <antwika/event/TickedEventDispatcher.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/simulation/EngineLoop.hpp>

#include "antwika/sudoku/BoardSink.hpp"
#include "antwika/sudoku/PlaySink.hpp"
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

        std::vector<std::reference_wrapper<ITickEventSink>> timedSinks{
            board, play, stopSignal};

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
            .commands = overlay.commands().size()};
    } // GCOVR_EXCL_STOP

} // namespace antwika::sudoku
