#include <chrono>
#include <iostream>
#include <memory>
#include <string>

#include <antwika/app/RunRecorded.hpp>
#include <antwika/app/WindowedHost.hpp>
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
#include <antwika/app/TickLimitSource.hpp>

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

using antwika::app::RecordedRun;
using antwika::app::WindowedHost;
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
    constexpr antwika::gfx::Size kWindowSize{
        .width = 720, .height = 800};

    void run(const RecordedRun &recorded)
    {
        const auto config =
            antwika::sudoku::loadConfigFileOrDefaults(
                antwika::app::assetPath("config.json"));

        const auto options =
            antwika::sudoku::sudokuOptionsFrom(recorded.commandLine);

        WindowedHost host(
            std::cout,
            Level::Info,
            {.gfx = antwika::gfx::makeSelectedBackend,
             .input = antwika::input::makeSelectedInputBackend},
            WindowedSessionDesc{
                .name = "Antwika Sudoku",
                .windowTitle = "Antwika Sudoku",
                .canvas = kWindowSize,
                .input =
                    {.coalescePointerMotion = true,
                     .stopOnKey = antwika::input::Key::Escape},
                .replayPath = recorded.options.replayPath});

        auto &logger = host.logger();
        auto &session = host.session();

        const antwika::sudoku::Translator translator{
            antwika::i18n::kDefaultLocale};

        const SudokuScene scene{translator};
        SystemSleeper sleeper;

        antwika::console::ConsolePicture consoleOverlay{session.canvas()};

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
            .consoleLoadEnabled = 
                antwika::console::consoleLoadPermitted(
                    recorded.options)});

        logger.log(
            Level::Info,
            "Finished with " + std::to_string(summary.filled)
                + " of 81 squares filled, saying "
                + std::string{antwika::i18n::nameOf<Messages>(
                    statusNameId(summary.status))});
    }
}

int main(int argc, char **argv)
{
    return antwika::app::runRecorded(
        argc,
        argv,
        "antwika_sudoku",
        run,
        antwika::sudoku::sudokuFlags());
}
