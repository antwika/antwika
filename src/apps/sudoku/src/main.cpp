#include <chrono>
#include <iostream>
#include <memory>
#include <string>

#include <antwika/app/ConsoleLogging.hpp>
#include <antwika/app/RunRecorded.hpp>
#include <antwika/gfx/SelectedBackend.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/WindowDesc.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/InputPipeline.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/SelectedInputBackend.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/app/WindowInputSource.hpp>
#include <antwika/time/SystemSleeper.hpp>

#include <antwika/app/AssetPath.hpp>
#include "antwika/sudoku/ConfigFile.hpp"
#include "antwika/sudoku/BoardOverlay.hpp"
#include "antwika/sudoku/Messages.hpp"
#include "antwika/sudoku/PuzzleFile.hpp"
#include "antwika/sudoku/PuzzleSource.hpp"
#include "antwika/sudoku/PuzzleState.hpp"
#include "antwika/sudoku/RenderSink.hpp"
#include "antwika/sudoku/Status.hpp"
#include "antwika/sudoku/Sudoku.hpp"
#include "antwika/sudoku/SudokuOptions.hpp"
#include "antwika/sudoku/SudokuScene.hpp"
#include <antwika/app/TickLimitSource.hpp>

using antwika::app::ConsoleLogging;
using antwika::app::RecordedRun;
using antwika::gfx::WindowDesc;
using antwika::input::InputEventCodec;
using antwika::input::InputPipeline;
using antwika::log::Level;
using antwika::replay::ReplaySource;
using antwika::app::WindowInputSource;
using antwika::sudoku::BoardOverlay;
using antwika::sudoku::Messages;
using antwika::sudoku::PuzzleSource;
using antwika::sudoku::PuzzleState;
using antwika::sudoku::RenderSink;
using antwika::sudoku::statusNameId;
using antwika::sudoku::SudokuScene;
using antwika::sudoku::SudokuSummary;
using antwika::app::TickLimitSource;
using antwika::time::SystemSleeper;

namespace
{
    // Square-ish, with room above the grid for the bar.
    // Also what a click is mapped against, never a reported size.
    // SudokuScene says why, and why this window is not resizable.
    constexpr antwika::gfx::Size kWindowSize{
        .width = 720, .height = 800};


    void run(const RecordedRun &recorded)
    {
        // The numbers the run reads off config.json, once.
        const auto config =
            antwika::sudoku::loadConfigFileOrDefaults(
                antwika::app::assetPath("config.json"));

        const auto options =
            antwika::sudoku::sudokuOptionsFrom(recorded.commandLine);

        ConsoleLogging logging(std::cout, Level::Info);
        auto &logger = logging.logger();

        const auto backend = antwika::gfx::makeSelectedBackend(logger);
        const auto inputBackend =
            antwika::input::makeSelectedInputBackend(logger);

        logger.log(
            Level::Info,
            "Antwika Sudoku on backend: "
                + std::string(backend->name()) + ", input: "
                + std::string(inputBackend->name()));

        const auto window = backend->createWindow(WindowDesc{
            .title = "Antwika Sudoku",
            .size = kWindowSize,
            .resizable = false});

        // Fixed here, and read from nowhere else.
        // The Solve button is as wide as its own label.
        // And the grid sits under whatever height the bar comes to.
        // So a language off the environment would move every square.
        // Changing it is this line, exactly as the window size is.
        const antwika::sudoku::Translator translator{
            antwika::i18n::kDefaultLocale};

        const SudokuScene scene{translator};
        SystemSleeper sleeper;

        ReplaySource fileSource(
            antwika::app::scriptedEvents(recorded.options.replayPath));

        const InputEventCodec codec;

        // Live input is attached only when there is no replay to run.
        // Movement is coalesced, since a layout reads one position.
        // Nothing here is painted by dragging.
        InputPipeline input(
            fileSource,
            *inputBackend,
            codec,
            {.readsDevice = !recorded.options.replayPath.has_value(),
             .coalescePointerMotion = true,
             .stopOnKey = antwika::input::Key::Escape});

        WindowInputSource windowed(input, *backend, window->id());

        // The puzzle goes into the stream ahead of the recorder.
        // So a recording carries the grid it was played on.
        PuzzleSource puzzled(
            windowed,
            antwika::sudoku::startingPuzzle(
                options.puzzlePath,
                recorded.options.replayPath.has_value()));

        TickLimitSource source(puzzled, options.maxTicks);

        const SudokuSummary summary = antwika::sudoku::bootstrap({
            .logger = logger,
            .eventSink = recorded.eventSink,
            .inputSource = source,
            .codec = codec,
            .translator = translator,
            .canvas = kWindowSize,
            .replayRecorder = recorded.replayRecorder,
            .extraSink =
                [&](const PuzzleState &, const BoardOverlay &overlay)
            {
                return std::make_unique<RenderSink>(
                    *window,
                    scene,
                    overlay,
                    sleeper,
                    std::chrono::milliseconds(
                        config.framePeriodMs));
            }});

        logger.log(
            Level::Info,
            "Finished with " + std::to_string(summary.filled)
                + " of 81 squares filled, saying "
                + std::string{antwika::i18n::nameOf<Messages>(
                    statusNameId(summary.status))});
    }
} // namespace

int main(int argc, char **argv)
{
    return antwika::app::runRecorded(
        argc,
        argv,
        "antwika_sudoku",
        run,
        antwika::sudoku::sudokuFlags());
}
