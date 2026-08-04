#include <chrono>
#include <iostream>
#include <memory>
#include <string>

#include <antwika/app/ConsoleLogging.hpp>
#include <antwika/app/RunRecorded.hpp>
#include <antwika/app/WindowedSession.hpp>
#include <antwika/console/ConsolePicture.hpp>
#include <antwika/console/SnapshotCommands.hpp>
#include <antwika/gfx/SelectedBackend.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/SelectedInputBackend.hpp>
#include <antwika/log/Level.hpp>
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
using antwika::app::WindowedSession;
using antwika::app::WindowedSessionDesc;
using antwika::log::Level;
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

        // Movement is coalesced, since a layout reads one position.
        // Nothing here is painted by dragging.
        const WindowedSessionDesc desc{
            .name = "Antwika Sudoku",
            .windowTitle = "Antwika Sudoku",
            .canvas = kWindowSize,
            .input =
                {.coalescePointerMotion = true,
                 .stopOnKey = antwika::input::Key::Escape},
            .replayPath = recorded.options.replayPath};

        WindowedSession session(logger, *backend, *inputBackend, desc);

        // Fixed here, and read from nowhere else.
        // The Solve button is as wide as its own label.
        // And the grid sits under whatever height the bar comes to.
        // So a language off the environment would move every square.
        // Changing it is this line, exactly as the window size is.
        const antwika::sudoku::Translator translator{
            antwika::i18n::kDefaultLocale};

        const SudokuScene scene{translator};
        SystemSleeper sleeper;

        // The console's picture, shared by the sink and the renderer.
        // Handing it over is what mounts the console at all.
        antwika::console::ConsolePicture consoleOverlay{session.canvas()};

        // The puzzle goes into the stream ahead of the recorder.
        // So a recording carries the grid it was played on.
        PuzzleSource puzzled(
            session.source(),
            antwika::sudoku::startingPuzzle(
                options.puzzlePath,
                recorded.options.replayPath.has_value()));

        TickLimitSource source(puzzled, options.maxTicks);

        const SudokuSummary summary = antwika::sudoku::bootstrap({
            .logger = logger,
            .eventSink = recorded.eventSink,
            .inputSource = source,
            .codec = session.codec(),
            .translator = translator,
            .canvas = kWindowSize,
            .replayRecorder = recorded.replayRecorder,
            .extraSink =
                [&](const PuzzleState &, const BoardOverlay &overlay)
            {
                return std::make_unique<RenderSink>(
                    session.window(),
                    scene,
                    overlay,
                    consoleOverlay,
                    sleeper,
                    std::chrono::milliseconds(
                        config.framePeriodMs));
            },
            .consoleOverlay = consoleOverlay,
            .consoleLoadEnabled = antwika::console::consoleLoadPermitted(
                recorded.options.recordPath.has_value(),
                recorded.options.replayPath.has_value())});

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
