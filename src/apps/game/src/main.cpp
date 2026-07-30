#include "antwika/game/Game.hpp"

#include <chrono>
#include <functional>
#include <iostream>
#include <vector>

#include <antwika/app/ConsoleLogging.hpp>
#include <antwika/app/PngFile.hpp>
#include <antwika/app/RunRecorded.hpp>
#include <antwika/ecs/ISystem.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/SelectedBackend.hpp>
#include <antwika/gfx/WindowDesc.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/InputPipeline.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/SelectedInputBackend.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/time/SystemSleeper.hpp>

#include "antwika/game/Camera.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/GridScene.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/RenderSystem.hpp"
#include "antwika/game/TickPacer.hpp"
#include "antwika/game/UiCanvas.hpp"
#include "antwika/game/UiOverlay.hpp"
#include "antwika/game/WindowInputSource.hpp"

using antwika::app::ConsoleLogging;
using antwika::app::RecordedRun;
using antwika::ecs::ISystem;
using antwika::game::Camera;
using antwika::game::GridExtent;
using antwika::game::GridScene;
using antwika::game::PathIndex;
using antwika::game::RenderSystem;
using antwika::game::TickPacer;
using antwika::game::UiOverlay;
using antwika::game::WindowInputSource;
using antwika::gfx::Point;
using antwika::gfx::WindowDesc;
using antwika::input::InputEventCodec;
using antwika::input::InputPipeline;
using antwika::log::Level;
using antwika::replay::ReplaySource;
using antwika::time::SystemSleeper;

namespace
{
    constexpr GridExtent kExtent{.width = 24, .height = 24};

    // The origin cell's top corner starts here.
    // The projection is anchored to the camera, not the canvas centre.
    // So this constant is what puts the grid in view.
    // And it is why a resize cannot change which cell a pixel means.
    constexpr Point kInitialPan{.x = 512, .y = 48};

    // 12.5 ticks per second, halved from the 25 it used to run at.
    // A walker crosses a cell per tick, and 25 was too fast to follow.
    constexpr std::chrono::milliseconds kTickInterval{80};

    // Escape ends a live run, and so does closing the window.
    // Neither is available under the headless backend.
    // That build therefore runs until it is interrupted.
    constexpr antwika::input::Key kQuitKey = antwika::input::Key::Escape;

    void run(const RecordedRun &recorded)
    {
        ConsoleLogging logging(std::cout, Level::Info);
        auto &logger = logging.logger();

        const auto backend = antwika::gfx::makeSelectedBackend(logger);
        const auto inputBackend =
            antwika::input::makeSelectedInputBackend(logger);
        logger.log(
            Level::Info,
            "Antwika Game on backends: " + std::string(backend->name())
                + " / " + std::string(inputBackend->name()));

        // Asked for the canvas the toolbar is resolved against.
        // Stating the two separately is what lets them disagree.
        const auto window = backend->createWindow(WindowDesc{
            .title = "Antwika Game",
            .size = antwika::game::kUiCanvas});

        const auto atlasBitmap = antwika::app::readPngFile(
            ANTWIKA_GAME_ATLAS_PATH, "antwika_game");

        // After the window, since a backend may have no device yet.
        // Declared after it too, so it is destroyed first.
        const auto atlas = window->renderer().createTexture(atlasBitmap);

        Camera camera(kInitialPan);
        PathIndex paths;
        const GridScene scene;

        // Against the size the window was asked for.
        // Never the size one reports, which nothing records.
        // That is what makes a recorded click hit the same button.
        UiOverlay overlay(antwika::game::kUiCanvas);
        RenderSystem renderSystem(
            *window, scene, *atlas, paths, camera, kExtent, overlay);
        SystemSleeper sleeper;
        TickPacer pacer(sleeper, kTickInterval);

        // Paced even under the backend that draws nothing.
        // That build used to stop after its scripted run.
        // An unbounded one would spin a core flat out instead.
        std::vector<std::reference_wrapper<ISystem>> observers{
            renderSystem, pacer};

        // Nothing is scripted unless a replay was asked for.
        // A plain run starts empty and builds only what gets clicked.
        ReplaySource fileSource(
            antwika::app::scriptedEvents(recorded.options.replayPath));

        const InputEventCodec codec;

        // A --replay run must not read a device.
        // Every input would arrive twice: from the file and the device.
        // Nothing else about the two branches differs, deliberately.
        InputPipeline input(
            fileSource,
            *inputBackend,
            codec,
            {.readsDevice = !recorded.options.replayPath.has_value(),
             .coalescePointerMotion = true,
             .thinIdleMotion = true,
             .stopOnKey = kQuitKey});

        // Both ways out are input, so both record and both replay.
        // A replay carrying its own stop simply gets a second one.
        // StopSignal ends the run on whichever arrives first.
        WindowInputSource source(input, *backend, window->id());

        antwika::game::printSummary(
            std::cout,
            antwika::game::bootstrap(antwika::game::GameConfig{
                .logger = logger,
                .eventSink = recorded.eventSink,
                .inputSource = source,
                .codec = codec,
                .extent = kExtent,
                .camera = camera,
                .paths = paths,
                .observers = observers,
                .replayRecorder = recorded.replayRecorder,
                .overlay = overlay}));
    }
} // namespace

int main(int argc, char **argv)
{
    return antwika::app::runRecorded(argc, argv, "antwika_game", run);
}
