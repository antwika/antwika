#include "antwika/game/Game.hpp"

#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <string_view>
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
#include <antwika/input/PointerHintChannel.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/SelectedInputBackend.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/time/SystemSleeper.hpp>

#include "antwika/game/AppMode.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/GridScene.hpp"
#include "antwika/game/MainMenuScene.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/RenderSystem.hpp"
#include "antwika/game/SaveCli.hpp"
#include "antwika/game/SaveDirectory.hpp"
#include "antwika/game/SaveGameFile.hpp"
#include "antwika/game/SaveLoadScene.hpp"
#include "antwika/game/TickPacer.hpp"
#include "antwika/game/UiCanvas.hpp"
#include "antwika/game/UiOverlay.hpp"
#include "antwika/game/WindowInputSource.hpp"
#include "antwika/game/WorldMap.hpp"
#include "antwika/game/WorldMapScene.hpp"
#include "antwika/game/WorldMapState.hpp"

using antwika::app::ConsoleLogging;
using antwika::app::RecordedRun;
using antwika::ecs::ISystem;
using antwika::game::AppModeState;
using antwika::game::Camera;
using antwika::game::GridExtent;
using antwika::game::GridScene;
using antwika::game::MainMenuScene;
using antwika::game::PathIndex;
using antwika::game::RenderSystem;
using antwika::game::SaveLoadScene;
using antwika::game::TickPacer;
using antwika::game::UiOverlay;
using antwika::game::WindowInputSource;
using antwika::game::WorldMapConfig;
using antwika::game::WorldMapScene;
using antwika::game::WorldMapState;
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

    constexpr std::chrono::milliseconds kTickInterval{40};

    // Escape ends a live run, and so does closing the window.
    // Neither is available under the headless backend.
    // That build therefore runs until it is interrupted.
    constexpr antwika::input::Key kQuitKey = antwika::input::Key::Escape;

    // The world is a pure function of this.
    // So a replay carries the number and the map comes back identical.
    // It is a constant rather than a flag.
    // A flag would let two runs of one recording be on two worlds.
    constexpr WorldMapConfig kWorld{.width = 24, .height = 16, .seed = 7};

    // Relative to wherever the binary was started from.
    // Listed once, before the loop, and never from inside a tick.
    // A directory read per tick would not replay -- see listSaveGames().
    constexpr std::string_view kSaveDirectory = "saves";

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
        const MainMenuScene menuScene;

        // A run opens at the main menu.
        // That is a mode of its own, not a window over a running grid.
        // Nothing on the command line changes it.
        // A --replay run boots into the same mode a live one does.
        // So what a recorded click means cannot depend on the flags.
        AppModeState mode;

        // Against the size the window was asked for.
        // Never the size one reports, which nothing records.
        // That is what makes a recorded click hit the same button.
        UiOverlay overlay(antwika::game::kUiCanvas);
        UiOverlay menuOverlay(antwika::game::kUiCanvas);
        UiOverlay saveOverlay(antwika::game::kUiCanvas);

        const SaveLoadScene saveScene;

        // Where the pointer is, off the event stream.
        // The one channel a replay does not reproduce.
        // Declared before both the renderer and the pipeline.
        antwika::input::PointerHintChannel hint;

        const WorldMapScene worldScene;
        WorldMapState cities(antwika::game::generateWorldMap(kWorld));

        RenderSystem renderSystem(antwika::game::RenderSetup{
            .window = *window,
            .mode = mode,
            .canvas = antwika::game::kUiCanvas,
            .scene = scene,
            .atlas = *atlas,
            .paths = paths,
            .camera = camera,
            .extent = kExtent,
            .overlay = overlay,
            .hint = hint,
            .menuScene = menuScene,
            .menuOverlay = menuOverlay,
            .saveScene = saveScene,
            .saveOverlay = saveOverlay,
            .worldScene = worldScene,
            .cities = cities});
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
             .pointerHint = hint,
             .stopOnKey = kQuitKey});

        // Both ways out are input, so both record and both replay.
        // A replay carrying its own stop simply gets a second one.
        // StopSignal ends the run on whichever arrives first.
        WindowInputSource source(input, *backend, window->id());

        const auto saveOptions =
            antwika::game::saveCliOptionsFrom(recorded.commandLine);

        const auto summary =
            antwika::game::bootstrap(antwika::game::GameConfig{
                .logger = logger,
                .eventSink = recorded.eventSink,
                .inputSource = source,
                .codec = codec,
                .extent = kExtent,
                .camera = camera,
                .paths = paths,
                .mode = mode,
                .observers = observers,
                .replayRecorder = recorded.replayRecorder,
                .overlay = overlay,
                .menuOverlay = menuOverlay,
                .world = cities,
                .saveOverlay = saveOverlay,
                .saves = antwika::game::listSaveGames(kSaveDirectory),
                .saveDirectory = std::string(kSaveDirectory),
                .start = antwika::game::loadGameFileIfNamed(
                    saveOptions.loadPath),
                .seed = kWorld.seed,
                .canvas = antwika::game::kUiCanvas});

        antwika::game::saveGameFileIfNamed(
            antwika::game::saveGameOf(summary, kExtent, kWorld.seed),
            saveOptions.savePath);

        antwika::game::printSummary(std::cout, summary);
    }
} // namespace

int main(int argc, char **argv)
{
    return antwika::app::runRecorded(
        argc, argv, "antwika_game", run, antwika::game::saveCliFlags());
}
