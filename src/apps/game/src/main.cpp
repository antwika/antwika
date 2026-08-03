#include "antwika/game/Game.hpp"

#include <chrono>
#include <cstdint>
#include <functional>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/app/AssetPath.hpp>
#include <antwika/app/ConsoleLogging.hpp>
#include <antwika/app/FramePacedSource.hpp>
#include <antwika/app/FullscreenToggleSource.hpp>
#include <antwika/app/PngFile.hpp>
#include <antwika/app/RunRecorded.hpp>
#include <antwika/app/WindowPointerMapping.hpp>
#include <antwika/ecs/ISystem.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/SelectedBackend.hpp>
#include <antwika/gfx/WindowDesc.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/InputPipeline.hpp>
#include <antwika/input/PointerHintChannel.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/SelectedInputBackend.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/simulation/WindowInputSource.hpp>
#include <antwika/time/SystemClock.hpp>
#include <antwika/time/SystemSleeper.hpp>

#include "antwika/game/AppMode.hpp"
#include "antwika/game/AtlasImage.hpp"
#include "antwika/game/BindingSource.hpp"
#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/FrameMeter.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/GridScene.hpp"
#include "antwika/game/KeyBindings.hpp"
#include "antwika/game/MainMenuScene.hpp"
#include "antwika/game/Messages.hpp"
#include "antwika/game/OptionsFile.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/PauseState.hpp"
#include "antwika/game/RenderSystem.hpp"
#include "antwika/game/RoadDrag.hpp"
#include "antwika/game/SaveCli.hpp"
#include "antwika/game/SaveDirectory.hpp"
#include "antwika/game/SaveGameFile.hpp"
#include "antwika/game/SaveLoadScene.hpp"
#include "antwika/game/UiCanvas.hpp"
#include "antwika/game/UiOverlay.hpp"
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
using antwika::game::UiOverlay;
using antwika::game::WorldMapConfig;
using antwika::game::WorldMapScene;
using antwika::game::WorldMapState;
using antwika::gfx::Point;
using antwika::gfx::WindowDesc;
using antwika::input::InputEventCodec;
using antwika::input::InputPipeline;
using antwika::log::Level;
using antwika::replay::ReplaySource;
using antwika::simulation::WindowInputSource;
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

    // How many frames one tick is shown as, at most.
    // Forty over a 40 ms tick is a thousand a second at 25 ticks.
    // So a walker crosses a cell in eighty frames, not in two jumps.
    //
    // **A ceiling rather than a quota, which is what makes it safe.**
    // Each frame is due at a fixed offset from the top of the tick.
    // One whose moment has gone by is dropped rather than drawn late.
    // So a machine that cannot draw this often draws less often.
    // The tick still lasts exactly kTickInterval either way.
    // See FramePacedSource.
    //
    // It was four, while the pacer slept a whole slice per frame.
    // A frame then cost a slice plus the sleeper's own overshoot.
    // Which is why a run asking for a hundred a second measured ninety.
    // And why raising it made the ticks slower rather than the picture.
    // Nothing about a frame can reach the simulation either way.
    constexpr std::uint32_t kFramesPerTick = 40;

    // Escape ends a live run, and so does closing the window.
    // Neither is available under the headless backend.
    // That build therefore runs until it is interrupted.
    //
    // Named in KeyBindings.hpp rather than here.
    // Along with the fullscreen key below.
    // Because the options screen has to refuse both.
    // A binding on either would fire and do this as well.
    constexpr antwika::input::Key kQuitKey = antwika::game::kQuitKey;

    // Fills the screen, and puts the window back.
    // An action on the window rather than anything a tick can see.
    // Which is why it is acted on above the loop and not in a sink.
    // A run reaches the same state whether or not it was ever pressed.
    constexpr antwika::input::Key kFullscreenKey =
        antwika::game::kFullscreenKey;

    // The world is a pure function of this.
    // So a replay carries the number and the map comes back identical.
    // It is a constant rather than a flag.
    // A flag would let two runs of one recording be on two worlds.
    constexpr WorldMapConfig kWorld{.width = 24, .height = 16, .seed = 7};

    // Relative to wherever the binary was started from.
    // Listed once, before the loop, and never from inside a tick.
    // A directory read per tick would not replay -- see listSaveGames().
    constexpr std::string_view kSaveDirectory = "saves";

    // Where this machine keeps which key asks for what.
    // Read once, before the loop, and never from inside a tick.
    // What it held is announced onto the wire and so is recorded.
    // Which is what replays a session on a machine bound otherwise.
    // See BindingSource.hpp.
    constexpr std::string_view kOptionsFile = "options.json";

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
        // Resizable, and F10 fills the screen, so they will.
        // What that changes is how big the picture is drawn and where.
        // Never what a click means -- see RenderSystem and docs/.
        const auto window = backend->createWindow(WindowDesc{
            .title = "Antwika Game",
            .size = antwika::game::kUiCanvas,
            .resizable = true});

        // One sheet per footprint size -- see TileAtlas.hpp.
        // Loaded with the sheet it must match named beside its file.
        // Nothing regenerates the art now.
        // So a wrong-sized export is an ordinary mistake.
        // Refused here, since a blit past the edge draws nothing.
        const auto loadAtlas = [](antwika::game::AtlasKind kind,
                                  std::string_view name)
        {
            auto bitmap = antwika::app::readPngFile(
                antwika::app::assetPath(name), "antwika_game");

            antwika::game::requireAtlasSize(bitmap, kind, name);

            return bitmap;
        };

        const auto atlas1x1Bitmap =
            loadAtlas(antwika::game::AtlasKind::OneByOne, "atlas_1x1.png");
        const auto atlas2x2Bitmap =
            loadAtlas(antwika::game::AtlasKind::TwoByTwo, "atlas_2x2.png");
        const auto atlas3x3Bitmap = loadAtlas(
            antwika::game::AtlasKind::ThreeByThree, "atlas_3x3.png");

        // After the window, since a backend may have no device yet.
        // Declared after it too, so they are destroyed first.
        const auto atlas1x1 =
            window->renderer().createTexture(atlas1x1Bitmap);
        const auto atlas2x2 =
            window->renderer().createTexture(atlas2x2Bitmap);
        const auto atlas3x3 =
            window->renderer().createTexture(atlas3x3Bitmap);

        // **One translator, at kDefaultLocale, fixed in source.**
        // Never from a flag and never from the environment.
        // This application lays its toolbar out from that text.
        // And it resolves a recorded click against that layout.
        // So the language may not be a thing a recording lacks.
        // Changing it is a source change, exactly as kUiCanvas is.
        const antwika::game::Translator translator{
            antwika::i18n::kDefaultLocale};

        Camera camera(kInitialPan);
        PathIndex paths;
        antwika::game::BuildingIndex built;
        const GridScene scene(translator);
        const MainMenuScene menuScene(translator);

        // A run opens at the main menu.
        // That is a mode of its own, not a window over a running grid.
        // Nothing on the command line changes it.
        // A --replay run boots into the same mode a live one does.
        // So what a recorded click means cannot depend on the flags.
        AppModeState mode;

        // Owned here rather than inside the run, as the mode is.
        // The renderer below reads it to draw a held walker still.
        antwika::game::PauseState pause;

        // Owned here for the pause's reason exactly.
        // The renderer below previews the run of road it names.
        antwika::game::RoadDrag drag;

        // Against the size the window was asked for.
        // Never the size one reports, which nothing records.
        // That is what makes a recorded click hit the same button.
        UiOverlay overlay(antwika::game::kUiCanvas);
        UiOverlay menuOverlay(antwika::game::kUiCanvas);
        UiOverlay saveOverlay(antwika::game::kUiCanvas);

        const SaveLoadScene saveScene(translator);

        // Where the pointer is, off the event stream.
        // The one channel a replay does not reproduce.
        // Declared before both the renderer and the pipeline.
        antwika::input::PointerHintChannel hint;

        const WorldMapScene worldScene;
        WorldMapState cities(antwika::game::generateWorldMap(kWorld));

        // The one wall clock in this application.
        // It reaches the frame meter and the pacer and nothing else.
        // One measures how often a frame is drawn, the other decides it.
        // No replay reproduces either, so nothing simulated may read it.
        const antwika::time::SystemClock clock;
        antwika::game::FrameMeter frameMeter(clock);

        RenderSystem renderSystem(antwika::game::RenderSetup{
            .window = *window,
            .mode = mode,
            .canvas = antwika::game::kUiCanvas,
            .scene = scene,
            .atlases =
                {.oneByOne = *atlas1x1,
                 .twoByTwo = *atlas2x2,
                 .threeByThree = *atlas3x3},
            .paths = paths,
            .built = built,
            .camera = camera,
            .extent = kExtent,
            .pause = pause,
            .overlay = overlay,
            .drag = drag,
            .hint = hint,
            .menuScene = menuScene,
            .menuOverlay = menuOverlay,
            .saveScene = saveScene,
            .saveOverlay = saveOverlay,
            .worldScene = worldScene,
            .cities = cities,
            .fps = frameMeter});
        SystemSleeper sleeper;

        // The pacing lives in the source now, not in an observer.
        // So this is only what draws the tick's own frame.
        std::vector<std::reference_wrapper<ISystem>> observers{
            renderSystem};

        // Nothing is scripted unless a replay was asked for.
        // A plain run starts empty and builds only what gets clicked.
        ReplaySource fileSource(
            antwika::app::scriptedEvents(recorded.options.replayPath));

        const InputEventCodec codec;

        // Where a window pixel is on the canvas, and nothing else.
        // Attached upstream of the recorder.
        // So a file holds canvas positions and replays under any size.
        const antwika::app::WindowPointerMapping mapping(
            *window, antwika::game::kUiCanvas);

        // A --replay run must not read a device.
        // Every input would arrive twice: from the file and the device.
        // Nothing else about the two branches differs, deliberately.
        InputPipeline input(
            fileSource,
            *inputBackend,
            codec,
            {.readsDevice = !recorded.options.replayPath.has_value(),
             .pointerMapping = mapping,
             .coalescePointerMotion = true,
             .thinIdleMotion = true,
             .pointerHint = hint,
             .stopOnKey = kQuitKey});

        // Both ways out are input, so both record and both replay.
        // A replay carrying its own stop simply gets a second one.
        // StopSignal ends the run on whichever arrives first.
        WindowInputSource source(input, *backend, window->id());

        // Above the loop, since filling the screen is not a tick's news.
        // The key press is ordinary recorded input all the same.
        // So a replay fills the screen where the run did, and agrees.
        antwika::app::FullscreenToggleSource fullscreen(
            source, *window, codec, kFullscreenKey);

        // Paced even under the backend that draws nothing.
        // An unbounded run would otherwise spin a core flat out.
        // The extra frames go in the gap before a tick's events arrive.
        // So a walker slides across a cell instead of jumping it.
        // A tick still takes exactly kTickInterval either way.
        antwika::app::FramePacedSource paced(
            fullscreen,
            renderSystem,
            sleeper,
            clock,
            {.tickInterval = kTickInterval,
             .framesPerTick = kFramesPerTick});

        // A replay gets neither the layout nor the path.
        // So it resolves recorded key presses against the recording.
        // And it leaves this machine's own bindings alone.
        const auto machine = antwika::game::machineOptionsFor(
            recorded.options.replayPath.has_value(),
            std::string(kOptionsFile));

        // Outermost, so what it announces is ahead of everything.
        // Upstream of the recorder, so a --record file carries it.
        antwika::game::BindingSource bound(paced, machine.bindings);

        const auto saveOptions =
            antwika::game::saveCliOptionsFrom(recorded.commandLine);

        // A loaded city reaches the session through no event.
        // A recording started from one replays against an empty grid.
        // So the pair is refused rather than recorded wrongly.
        antwika::game::requireRecordableStart(
            saveOptions, recorded.options.recordPath.has_value());

        const auto summary =
            antwika::game::bootstrap(antwika::game::GameConfig{
                .logger = logger,
                .eventSink = recorded.eventSink,
                .inputSource = bound,
                .codec = codec,
                .extent = kExtent,
                .camera = camera,
                .paths = paths,
                .built = built,
                .mode = mode,
                .pause = pause,
                .drag = drag,
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
                .savePath = saveOptions.savePath,
                .optionsPath = machine.path,
                .seed = kWorld.seed,
                .translator = translator,
                .canvas = antwika::game::kUiCanvas});

        antwika::game::printSummary(std::cout, summary);
    }
} // namespace

int main(int argc, char **argv)
{
    return antwika::app::runRecorded(
        argc, argv, "antwika_game", run, antwika::game::saveCliFlags());
}
