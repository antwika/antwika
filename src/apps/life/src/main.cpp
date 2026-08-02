#include "antwika/life/Life.hpp"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>

#include <antwika/app/AssetPath.hpp>
#include <antwika/app/ConsoleLogging.hpp>
#include <antwika/app/RunRecorded.hpp>
#include <antwika/gfx/SelectedBackend.hpp>
#include <antwika/gfx/WindowDesc.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/InputPipeline.hpp>
#include <antwika/input/SelectedInputBackend.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/simulation/TickPacer.hpp>
#include <antwika/simulation/WindowInputSource.hpp>
#include <antwika/time/SystemSleeper.hpp>

#include "antwika/life/BoardScene.hpp"
#include "antwika/life/PointerToggleSink.hpp"
#include "antwika/life/PrintSystem.hpp"
#include "antwika/life/RenderSystem.hpp"

using antwika::app::ConsoleLogging;
using antwika::app::RecordedRun;
using antwika::ecs::World;
using antwika::gfx::WindowDesc;
using antwika::input::InputEventCodec;
using antwika::input::InputPipeline;
using antwika::life::BoardScene;
using antwika::life::DragState;
using antwika::life::Grid;
using antwika::life::PointerToggleSink;
using antwika::life::PrintSystem;
using antwika::life::RenderSystem;
using antwika::log::Level;
using antwika::replay::ReplaySource;
using antwika::simulation::TickPacer;
using antwika::simulation::WindowInputSource;
using antwika::time::SystemSleeper;

namespace
{
    constexpr std::uint32_t kBoardWidth = 32;
    constexpr std::uint32_t kBoardHeight = 32;

    // Square, and a whole number of pixels per cell at this board size.
    // Also what a click is mapped against, not the size a window reports.
    // PointerToggleSink says why, and why the window is not resizable.
    constexpr antwika::gfx::Size kWindowSize{.width = 768, .height = 768};

    constexpr std::chrono::milliseconds kTickInterval{50};

    // The backend that draws nothing.
    // A build using it has nothing to watch and nothing to wait for.
    constexpr std::string_view kHeadlessBackendName = "null";

    void run(const RecordedRun &recorded)
    {
        ConsoleLogging logging(std::cout, Level::Info);
        auto &logger = logging.logger();

        const auto backend = antwika::gfx::makeSelectedBackend(logger);
        const auto inputBackend =
            antwika::input::makeSelectedInputBackend(logger);
        const bool drawsNothing = backend->name() == kHeadlessBackendName;

        logger.log(
            Level::Info,
            "Antwika Life on backend: " + std::string(backend->name())
                + ", input: " + std::string(inputBackend->name()));
        antwika::life::announceHowToStop(logger, drawsNothing);

        const auto window = backend->createWindow(WindowDesc{
            .title = "Antwika Life",
            .size = kWindowSize,
            .resizable = false});

        const BoardScene scene;
        RenderSystem renderSystem(*window, scene, kBoardWidth, kBoardHeight);
        PrintSystem printSystem(kBoardWidth, std::cout);
        SystemSleeper sleeper;
        TickPacer pacer(sleeper, kTickInterval);

        ReplaySource fileSource(antwika::app::scriptedEvents(
            recorded.options.replayPath,
            antwika::app::assetPath("demo.jsonl")));

        const InputEventCodec codec;

        // Live input is attached only when there is no replay to run.
        // A replay already holds the input it recorded.
        // Reading a device too would make every event arrive twice.
        // Idle movement toggles nothing, so it is held back.
        // Coalescing is deliberately off here.
        // A drag toggles every cell it crosses.
        // Thinning a run of movement inside a tick would skip cells.
        InputPipeline input(
            fileSource,
            *inputBackend,
            codec,
            {.readsDevice = !recorded.options.replayPath.has_value(),
             .coalescePointerMotion = false,
             .thinIdleMotion = true});

        WindowInputSource source(input, *backend, window->id());

        antwika::life::bootstrap(antwika::life::LifeConfig{
            .logger = logger,
            .eventSink = recorded.eventSink,
            .inputSource = source,
            .width = kBoardWidth,
            .height = kBoardHeight,
            .observers = antwika::life::observersFor(
                renderSystem, printSystem, pacer, drawsNothing),
            .replayRecorder = recorded.replayRecorder,
            .extraSink =
                [&codec](World &world, const Grid &grid, DragState &drag)
            {
                return std::make_unique<PointerToggleSink>(
                    world, grid, codec, kWindowSize, drag);
            }});
    }
} // namespace

int main(int argc, char **argv)
{
    return antwika::app::runRecorded(argc, argv, "antwika_life", run);
}
