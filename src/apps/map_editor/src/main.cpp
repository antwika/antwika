#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <variant>
#include <iostream>
#include <string>
#include <string_view>

#include <antwika/app/AssetPath.hpp>
#include <antwika/app/ConsoleLogging.hpp>
#include <antwika/app/RunGuarded.hpp>
#include <antwika/cli/CommandLine.hpp>
#include <antwika/ecs/SystemScheduler.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/gfx/SelectedBackend.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/ViewportRenderer.hpp>
#include <antwika/gfx/WindowDesc.hpp>
#include <antwika/gfx/WindowEvent.hpp>
#include <antwika/input/SelectedInputBackend.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/time/SystemSleeper.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/map_editor/BrushSystem.hpp"
#include "antwika/map_editor/CharacterSheets.hpp"
#include "antwika/map_editor/ConfigFile.hpp"
#include "antwika/map_editor/EditorConsole.hpp"
#include "antwika/map_editor/EditorStore.hpp"
#include "antwika/map_editor/EntityEditSystem.hpp"
#include "antwika/map_editor/GenerationRules.hpp"
#include "antwika/map_editor/KeyboardSystem.hpp"
#include "antwika/map_editor/MapRenderSystem.hpp"
#include "antwika/map_editor/MirrorSystem.hpp"
#include "antwika/map_editor/PointerSystem.hpp"
#include "antwika/map_editor/SheetWorkspace.hpp"
#include "antwika/map_editor/UiSystem.hpp"
#include "antwika/map_editor/ValidationSystem.hpp"

using antwika::app::ConsoleLogging;
using antwika::app::runGuarded;
using antwika::ecs::SystemScheduler;
using antwika::ecs::World;
using antwika::gfx::ViewportRenderer;
using antwika::gfx::WindowDesc;
using antwika::log::Level;
using antwika::map_editor::BrushSystem;
using antwika::map_editor::EditorConsoleSystem;
using antwika::map_editor::EditorStore;
using antwika::map_editor::EntityEditSystem;
using antwika::map_editor::KeyboardSystem;
using antwika::map_editor::MapRenderSystem;
using antwika::map_editor::MirrorSystem;
using antwika::map_editor::PointerSystem;
using antwika::map_editor::UiSystem;
using antwika::map_editor::ValidationSystem;

namespace
{
    constexpr std::chrono::milliseconds kFramePeriod{16};

    constexpr std::string_view kName = "antwika_map_editor";

    constexpr std::string_view kMapFlag = "--map";

    constexpr std::string_view kDefaultPath = "map.json";

    constexpr std::string_view kTilesFlag = "--tiles";

    constexpr std::string_view kDefaultTiles = "assets/tiles";

    constexpr std::string_view kCharactersFlag = "--characters";

    constexpr std::string_view kDefaultCharacters =
        "assets/characters";

    constexpr std::array kFlags = {
        antwika::cli::FlagSpec{
            .name = kMapFlag,
            .valueName = "path",
            .help = "Load and save the map at this path."},
        antwika::cli::FlagSpec{
            .name = kTilesFlag,
            .valueName = "dir",
            .help = "Load and save terrain sheets in this directory."},
        antwika::cli::FlagSpec{
            .name = kCharactersFlag,
            .valueName = "dir",
            .help = "Load and save character sheets in this directory."},
    };

    constexpr antwika::gfx::Size kCanvas{.width = 480, .height = 270};
}

int main(int argc, char **argv)
{
    ConsoleLogging logging(std::cout, Level::Info);
    auto &logger = logging.logger();

    return runGuarded(
        kName,
        [&logger, argc, argv]
        {
            const auto command =
                antwika::cli::parseCommandLine(argc, argv, kFlags);

            if (command.has(antwika::cli::kHelpFlag))
            {
                std::cout << antwika::cli::helpText(kName, kFlags);
                return;
            }

            EditorStore store{
                .state = antwika::map_editor::makeEditorState(
                    std::filesystem::path(
                        command.value(kMapFlag)
                            .value_or(std::string(kDefaultPath))),
                    logger)};

            store.tiles.directory = std::filesystem::path(
                command.value(kTilesFlag)
                    .value_or(std::string(kDefaultTiles)));

            store.state.rules =
                antwika::map_editor::loadRulesFileOrDefaults(
                    store.tiles.directory / "rules.json", logger);

            for (const auto terrain :
                 antwika::enums::kAll<antwika::tilemap::TerrainClass>)
            {
                store.tiles.docs[antwika::enums::index(terrain)]
                    .image = antwika::map_editor::loadSheetOrPlaceholder(
                    store.tiles.directory, terrain, logger);
            }

            store.tiles.connectors =
                antwika::map_editor::loadConnectorsFile(
                    store.tiles.directory);

            store.characters.directory = std::filesystem::path(
                command.value(kCharactersFlag)
                    .value_or(std::string(kDefaultCharacters)));
            store.characters.list =
                antwika::map_editor::loadCharacters(
                    store.characters.directory, logger);
            store.characters.nameField.text = "player";
            store.characters.nameField.cursor =
                store.characters.nameField.text.size();

            const auto config =
                antwika::map_editor::loadConfigFileOrDefaults(
                    antwika::app::assetPath("config.json"));

            store.uiScale = std::clamp(config.uiScale, 2U, 4U);

            const auto backend =
                antwika::gfx::makeSelectedBackend(logger);
            const auto input =
                antwika::input::makeSelectedInputBackend(logger);

            const auto window = backend->createWindow(WindowDesc{
                .title = "Wakewater map editor",
                .size =
                    {.width = kCanvas.width * store.uiScale,
                     .height = kCanvas.height * store.uiScale},
                .resizable = true});

            if (config.fullscreen)
            {
                window->setFullscreen(true);
            }

            store.fullscreen = window->isFullscreen();
            store.windowSize = window->size();

            ViewportRenderer view(
                window->renderer(), window->size(), kCanvas);

            World world(logger);
            SystemScheduler scheduler;

            EditorConsoleSystem consoleSystem(store, logger);
            PointerSystem pointerSystem(store, view);
            KeyboardSystem keyboardSystem(store, *window, logger);
            EntityEditSystem entityEditSystem(store);
            BrushSystem brushSystem(store);
            MirrorSystem mirrorSystem(store);
            ValidationSystem validationSystem(store);
            MapRenderSystem mapRenderSystem(store, view);
            UiSystem uiSystem(
                store,
                view,
                *window,
                kCanvas,
                consoleSystem.picture(),
                logger);

            const auto inputPhase = scheduler.createPhase("input");
            scheduler.addSystem(inputPhase, consoleSystem);
            scheduler.addSystem(inputPhase, pointerSystem);
            scheduler.addSystem(inputPhase, keyboardSystem);

            const auto editPhase = scheduler.createPhase("edit");
            scheduler.addSystem(editPhase, entityEditSystem);
            scheduler.addSystem(editPhase, brushSystem);

            const auto mirrorPhase = scheduler.createPhase("mirror");
            scheduler.addSystem(mirrorPhase, mirrorSystem);

            const auto validatePhase =
                scheduler.createPhase("validate");
            scheduler.addSystem(validatePhase, validationSystem);

            const auto renderPhase = scheduler.createPhase("render");
            scheduler.addSystem(renderPhase, mapRenderSystem);

            const auto uiPhase = scheduler.createPhase("ui");
            scheduler.addSystem(uiPhase, uiSystem);

            antwika::time::SystemSleeper sleeper;
            antwika::time::Tick tick = 0;

            while (window->isOpen())
            {
                bool closeRequested = false;

                while (const auto event = backend->pollEvent())
                {
                    if (event->window != window->id())
                    {
                        continue;
                    }

                    if (std::holds_alternative<
                            antwika::gfx::CloseRequested>(
                            event->payload))
                    {
                        closeRequested = true;
                    }

                    if (const auto *resized =
                            std::get_if<antwika::gfx::Resized>(
                                &event->payload))
                    {
                        view.resize(resized->size);
                        store.windowSize = resized->size;
                    }
                }

                if (closeRequested)
                {
                    window->close();
                    break;
                }

                store.input.events.clear();

                while (const auto event = input->pollEvent())
                {
                    store.input.events.push_back(event.value());
                }

                scheduler.run(world, tick);

                if (store.input.quit || !window->isOpen())
                {
                    break;
                }

                ++tick;
                sleeper.sleep(kFramePeriod);
            }

            window->close();

            logger.log(Level::Info, "Wakewater map editor closed");
        });
}
