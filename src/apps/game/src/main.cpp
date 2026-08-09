#include <chrono>
#include <cstdint>
#include <functional>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/app/AssetPath.hpp>
#include <antwika/app/ConsoleLogging.hpp>
#include <antwika/app/FramePacingTrace.hpp>
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
#include <antwika/app/WindowInputSource.hpp>
#include <antwika/time/SystemClock.hpp>
#include <antwika/time/SystemSleeper.hpp>
#include <antwika/console/ConsolePicture.hpp>
#include <antwika/console/SnapshotCommands.hpp>

#include "antwika/game/Game.hpp"
#include "antwika/game/AppMode.hpp"
#include "antwika/game/AtlasSheets.hpp"
#include "antwika/game/BindingSource.hpp"
#include "antwika/game/LocaleSource.hpp"
#include "antwika/game/LocaleState.hpp"
#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/ConfigFile.hpp"
#include "antwika/game/Desirability.hpp"
#include "antwika/game/FrameMeter.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/GridScene.hpp"
#include "antwika/game/KeyBindings.hpp"
#include "antwika/game/KeyboardSource.hpp"
#include "antwika/game/MainMenuScene.hpp"
#include "antwika/game/MapView.hpp"
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
using antwika::app::WindowInputSource;
using antwika::time::SystemSleeper;

namespace
{
    constexpr GridExtent kExtent{.width = 24, .height = 24};

    constexpr Point kInitialPan{.x = 512, .y = 48};

    constexpr std::chrono::milliseconds kTickInterval{40};

    constexpr std::uint32_t kFramesPerTick = 400;

    constexpr std::chrono::milliseconds kPacingWindow{1000};

    constexpr antwika::input::Key kQuitKey = antwika::game::kQuitKey;

    constexpr antwika::input::Key kFullscreenKey =
        antwika::game::kFullscreenKey;

    constexpr WorldMapConfig kWorld{.width = 24, .height = 16, .seed = 7};

    constexpr std::string_view kSaveDirectory = "saves";

    constexpr std::string_view kOptionsFile = "options.json";

    void run(const RecordedRun &recorded)
    {
        const auto config = antwika::game::loadConfigFileOrDefaults(
            antwika::app::assetPath("config.json"));

        ConsoleLogging logging(std::cout, Level::Info);
        auto &logger = logging.logger();

        const auto backend = antwika::gfx::makeSelectedBackend(logger);
        const auto inputBackend =
            antwika::input::makeSelectedInputBackend(logger);
        logger.log(
            Level::Info,
            "Antwika Game on backends: " + std::string(backend->name())
                + " / " + std::string(inputBackend->name()));

        const auto window = backend->createWindow(WindowDesc{
            .title = "Antwika Game",
            .size = antwika::game::kUiCanvas,
            .resizable = true});

        const auto sheets =
            antwika::game::loadAtlasSheets(config.atlases);

        const auto atlas1x1 = window->renderer().createTexture(
            sheets.of(antwika::game::AtlasKind::OneByOne));
        const auto atlas2x2 = window->renderer().createTexture(
            sheets.of(antwika::game::AtlasKind::TwoByTwo));
        const auto atlas3x3 = window->renderer().createTexture(
            sheets.of(antwika::game::AtlasKind::ThreeByThree));
        const auto walkerAtlas =
            window->renderer().createTexture(sheets.walker);

        antwika::game::LocaleState localeState;
        const antwika::game::Translator &translator =
            localeState.translator();

        Camera camera(kInitialPan);
        PathIndex paths;
        antwika::game::BuildingIndex built;
        const GridScene scene(translator);
        const MainMenuScene menuScene(translator);

        AppModeState mode;

        antwika::game::PauseState pause;

        antwika::game::RoadDrag drag;

        antwika::game::MapViewState mapView;

        antwika::game::DesirabilityField desirability;

        UiOverlay overlay(antwika::game::kUiCanvas);
        UiOverlay menuOverlay(antwika::game::kUiCanvas);
        UiOverlay saveOverlay(antwika::game::kUiCanvas);
        antwika::console::ConsolePicture consoleOverlay(
            antwika::game::kUiCanvas);

        const SaveLoadScene saveScene(translator);

        antwika::input::PointerHintChannel hint;

        const WorldMapScene worldScene;
        WorldMapState cities(antwika::game::generateWorldMap(kWorld));

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
                 .threeByThree = *atlas3x3,
                 .walker = *walkerAtlas,
                 .specs = sheets.specs},
            .paths = paths,
            .built = built,
            .camera = camera,
            .extent = kExtent,
            .pause = pause,
            .overlay = overlay,
            .view = mapView,
            .desirability = desirability,
            .drag = drag,
            .hint = hint,
            .menuScene = menuScene,
            .menuOverlay = menuOverlay,
            .saveScene = saveScene,
            .saveOverlay = saveOverlay,
            .consoleOverlay = consoleOverlay,
            .worldScene = worldScene,
            .cities = cities,
            .fps = frameMeter});
        SystemSleeper sleeper;

        std::vector<std::reference_wrapper<ISystem>> observers{
            renderSystem};

        ReplaySource fileSource(
            antwika::app::scriptedEvents(recorded.options.replayPath));

        const InputEventCodec codec;

        const antwika::app::WindowPointerMapping mapping(
            *window, antwika::game::kUiCanvas);

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

        WindowInputSource source(input, *backend, window->id());

        antwika::app::FullscreenToggleSource fullscreen(
            source, *window, codec, kFullscreenKey);

        antwika::app::FramePacingTrace pacingTrace(
            clock, logger, kPacingWindow);

        antwika::app::FramePacedSource paced(
            fullscreen,
            renderSystem,
            sleeper,
            clock,
            {.tickInterval = kTickInterval,
             .framesPerTick = kFramesPerTick},
            input.framePump(),
            pacingTrace);

        const auto machine = antwika::game::machineOptionsFor(
            recorded.options.replayPath.has_value(),
            std::string(kOptionsFile));

        antwika::game::BindingSource bound(paced, machine.bindings);

        antwika::game::LocaleSource localised(bound, machine.locale);

        antwika::game::KeyboardSource typed(localised, machine.keyboard);

        const auto saveOptions =
            antwika::game::saveCliOptionsFrom(recorded.commandLine);

        antwika::game::requireRecordableStart(
            saveOptions, recorded.options.recordPath.has_value());

        const auto summary =
            antwika::game::bootstrap(antwika::game::GameWiring{
                .logger = logger,
                .eventSink = recorded.eventSink,
                .inputSource = typed,
                .codec = codec,
                .extent = kExtent,
                .camera = camera,
                .paths = paths,
                .built = built,
                .mode = mode,
                .pause = pause,
                .view = mapView,
                .desirability = desirability,
                .drag = drag,
                .observers = observers,
                .replayRecorder = recorded.replayRecorder,
                .overlay = overlay,
                .menuOverlay = menuOverlay,
                .world = cities,
                .saveOverlay = saveOverlay,
                .consoleOverlay = consoleOverlay,
                .consoleLoadEnabled = 
                    antwika::console::consoleLoadPermitted(
                        recorded.options),
                .saves = antwika::game::listSaveGames(kSaveDirectory),
                .saveDirectory = std::string(kSaveDirectory),
                .start = antwika::game::loadGameFileIfNamed(
                    saveOptions.loadPath),
                .savePath = saveOptions.savePath,
                .optionsPath = machine.path,
                .seed = kWorld.seed,
                .locale = localeState,
                .canvas = antwika::game::kUiCanvas,
                .config = config});

        antwika::game::printSummary(std::cout, summary);
    }
}

int main(int argc, char **argv)
{
    return antwika::app::runRecorded(
        argc, argv, "antwika_game", run, antwika::game::saveCliFlags());
}
