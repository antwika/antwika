#include "antwika/life/ConfigFile.hpp"
#include "antwika/life/Life.hpp"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>

#include <antwika/app/AssetPath.hpp>
#include <antwika/app/ConsoleLogging.hpp>
#include <antwika/app/RunRecorded.hpp>
#include <antwika/app/WindowedSession.hpp>
#include <antwika/console/ConsolePicture.hpp>
#include <antwika/console/SnapshotCommands.hpp>
#include <antwika/gfx/SelectedBackend.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/input/SelectedInputBackend.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/simulation/TickPacer.hpp>
#include <antwika/time/SystemSleeper.hpp>

#include "antwika/life/BoardScene.hpp"
#include "antwika/life/PointerToggleSink.hpp"
#include "antwika/life/PrintSystem.hpp"
#include "antwika/life/RenderSystem.hpp"

using antwika::app::ConsoleLogging;
using antwika::app::RecordedRun;
using antwika::app::WindowedSession;
using antwika::app::WindowedSessionDesc;
using antwika::ecs::World;
using antwika::life::BoardScene;
using antwika::life::DragState;
using antwika::life::Grid;
using antwika::life::PointerToggleSink;
using antwika::life::PrintSystem;
using antwika::life::RenderSystem;
using antwika::log::Level;
using antwika::simulation::TickPacer;
using antwika::time::SystemSleeper;

namespace
{
    constexpr std::uint32_t kBoardWidth = 32;
    constexpr std::uint32_t kBoardHeight = 32;

    // Square, and a whole number of pixels per cell at this board size.
    // Also what a click is mapped against, not the size a window reports.
    // PointerToggleSink says why, and why the window is not resizable.
    constexpr antwika::gfx::Size kWindowSize{.width = 768, .height = 768};

    void run(const RecordedRun &recorded)
    {
        // The numbers the run reads off config.json, once.
        const auto config =
            antwika::life::loadConfigFileOrDefaults(
                antwika::app::assetPath("config.json"));

        ConsoleLogging logging(std::cout, Level::Info);
        auto &logger = logging.logger();

        const auto backend = antwika::gfx::makeSelectedBackend(logger);
        const auto inputBackend =
            antwika::input::makeSelectedInputBackend(logger);

        // Coalescing is deliberately off here.
        // A drag toggles every cell it crosses.
        // Thinning a run of movement inside a tick would skip cells.
        // Idle movement toggles nothing, so it is held back.
        const WindowedSessionDesc desc{
            .name = "Antwika Life",
            .windowTitle = "Antwika Life",
            .canvas = kWindowSize,
            .input =
                {.coalescePointerMotion = false, .thinIdleMotion = true},
            .replayPath = recorded.options.replayPath,
            .demoReplay = antwika::app::assetPath("demo.jsonl")};

        WindowedSession session(logger, *backend, *inputBackend, desc);

        antwika::life::announceHowToStop(logger, session.drawsNothing());

        const BoardScene scene;

        // Against the size the window was asked for.
        // Never the size one reports, which nothing records.
        antwika::console::ConsolePicture consoleOverlay(session.canvas());

        RenderSystem renderSystem(
            session.window(),
            scene,
            kBoardWidth,
            kBoardHeight,
            consoleOverlay);
        PrintSystem printSystem(kBoardWidth, std::cout);
        SystemSleeper sleeper;
        TickPacer pacer(
            sleeper,
            std::chrono::milliseconds(config.tickIntervalMs));

        antwika::life::bootstrap(antwika::life::LifeWiring{
            .logger = logger,
            .eventSink = recorded.eventSink,
            .inputSource = session.source(),
            .width = kBoardWidth,
            .height = kBoardHeight,
            .observers = antwika::life::observersFor(
                renderSystem,
                printSystem,
                pacer,
                session.drawsNothing()),
            .replayRecorder = recorded.replayRecorder,
            .extraSink =
                [&session](World &world, const Grid &grid, DragState &drag)
            {
                return std::make_unique<PointerToggleSink>(
                    world, grid, session.codec(), kWindowSize, drag);
            },
            .consoleOverlay = consoleOverlay,
            .consoleLoadEnabled = antwika::console::consoleLoadPermitted(
                recorded.options.recordPath.has_value(),
                recorded.options.replayPath.has_value())});
    }
} // namespace

int main(int argc, char **argv)
{
    return antwika::app::runRecorded(argc, argv, "antwika_life", run);
}
