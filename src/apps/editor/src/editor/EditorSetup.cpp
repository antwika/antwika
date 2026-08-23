#include <cmath>
#include <filesystem>
#include <numbers>

#include <antwika/component/AnimationState.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/ecs/OpenPhase.hpp>
#include <antwika/editor/ui/IconSheet.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/gfx/SelectedBackend.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/WindowSpec.hpp>
#include <antwika/gfx/WindowEvent.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/SelectedInputBackend.hpp>
#include <antwika/light/PointLight.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/map/MapAssets.hpp>
#include <antwika/map/MapFileError.hpp>
#include <antwika/render/Checkerboard.hpp>
#include <antwika/solver/VoxelWeave.hpp>
#include <antwika/tilemap/Tilemap.hpp>
#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/voxelmap/VoxelPick.hpp>
#include <antwika/app/WindowEvents.hpp>

#include "antwika/editor/Editor.hpp"
#include "antwika/editor/plan/PlanFileError.hpp"

namespace
{

}

namespace antwika::editor
{

    Editor::Editor(
        log::ILogger &logger,
        gfx::IGfxBackend &backendGiven,
        input::IInputBackend &inputsGiven,
        std::string mapPathGiven,
        const bool playOnlyGiven,
        std::string planPathGiven)
        : logger(logger),
          mapPath(std::move(mapPathGiven)),
          startMapPath(mapPath),
          playOnly(playOnlyGiven),
          backend(backendGiven),
          inputs(inputsGiven),
          window(
              backend.createWindow(
                  gfx::WindowSpec{
                      .title = "Antwika",
                      .size = app::kDefaultWindowSize,
                      .resizable = true})),
          viewportRenderer(
              window->renderer(), window->size(), camera::kCanvasSize),
          map{.voxels = voxel::expandCubesToVoxels(voxelmap::demoCells()),
              .tilemap = tilemap::defaultTilemap()},
          world(logger),
          game(logger, world, worldMeshes.cells(), patrolPositions)
    {
        worldShader.open(viewportRenderer, map::loadShader("voxel"));
        setBindings(loadChords(chordsPath()));

        try
        {
            map = map::loadMap(mapPath);

            logger.log(log::Level::Info, "Loaded " + mapPath);
        }
        catch (const map::MapFileError &)
        {
            logger.log(
                log::Level::Info,
                "No map at " + mapPath
                    + ", starting from the built-in one");
        }

        try
        {
            plan.open(std::move(planPathGiven));
        }
        catch (const PlanFileError &error)
        {
            logger.log(log::Level::Warning, error.what());
        }

        editLevel =
            antwika::voxel::cubeIndexOfLevel(voxelmap::topLevel(map.voxels));
        rebuildWorld();

        atlasSheets.open(
            viewportRenderer,
            map::loadAtlasPairOrBlank(mapPath, kAppName),
            map,
            tick);
        characterView.open(
            viewportRenderer, map::loadCharacterSheet(mapPath, kAppName));
        iconsView.open(
            viewportRenderer, loadIconSheet(mapPath, kAppName));
        sprites.open(viewportRenderer);
        lightPasses.open(viewportRenderer, map::loadShader("shadow"));
        scenePass.open(viewportRenderer, map::loadShader("bloom"));

        spawnRoster();
        loadCharacterSkins();

        const auto opening = map::CameraView{
            .transform = camera::centeredOn(camera::defaultTransform(),
                voxelmap::voxelsCenter(map.voxels))};

        cameraView = map.camera.value_or(opening);
        viewHeight =
            camera::orthoHalfHeight(camera::kCanvasSize, cameraView.zoom);

        activeView = map.settings.view;
        viewBeforePlay = map.settings.view;
        tool = map.settings.tool;
        lighting = map.settings.lighting;
        showRuleLines = map.settings.showRuleLines;
        paintMode = map.settings.paint;
        brushKind = map.settings.kind;
        grid = map.settings.grid;
        showPlacementGhost = map.settings.showPlacementGhost;
        lampSight = map.settings.lampSight;
        cameraFollows = map.settings.cameraFollows;
        hideAboveLevel = map.settings.hideAboveLevel;
        cornerJoining = map.settings.cornersJoined
                      ? solver::CornerSeams::Included
                      : solver::CornerSeams::Ignored;

        if (playOnly)
        {
            const auto progress = map::loadProgress(progressPath());

            if (progress.has_value())
            {
                mapPath = (std::filesystem::path(startMapPath)
                               .parent_path()
                           / progress->map)
                              .string();

                if (loadCurrentMap())
                {
                    {
                        const ecs::OpenPhase phase(game->world());

                        game->world().set<component::Position>(
                            game->player(),
                            collision::positionFrom(
                                progress->stancePlacement.position));
                        game->world().set<component::AnimationState>(
                            game->player(),
                            component::AnimationState{
                                .direction =
                                    progress->stancePlacement.way});
                    }

                    game->cameraTarget() =
                        progress->stancePlacement.position;
                }
                else
                {
                    mapPath = startMapPath;
                }
            }

            playing = true;
            activeView = map::View::World;
            titleScreenUp = true;
            aimPlayCamera();
        }

        logger.log(log::Level::Info, "Antwika opened");
    }

    bool Editor::pollWindow()
    {
        const auto changes = app::windowChanges(backend, window->id());

        if (changes.resizedSize.has_value())
        {
            viewportRenderer.resize(*changes.resizedSize);
        }

        const auto closeRequested = changes.closeRequested;

        if (closeRequested && dirty)
        {
            dialogs.quitConfirmOpen = true;

            return true;
        }

        return !closeRequested;
    }

    void Editor::pollInputs()
    {
        inputState.beginTick();

        while (const auto event = inputs.pollEvent())
        {
            inputState.apply(*event);

            if (const auto *rolled =
                    std::get_if<input::PointerScrolled>(&event.value()))
            {
                onScrolled(*rolled);
                continue;
            }

            if (const auto *movedEvent =
                    std::get_if<input::PointerMoved>(&event.value()))
            {
                onPointerMoved(*movedEvent);
                continue;
            }

            if (const auto *pressedEvent =
                    std::get_if<input::PointerButtonPressed>(
                    &event.value()))
            {
                onPointerPressed(*pressedEvent);
                continue;
            }

            if (const auto *releasedEvent =
                    std::get_if<input::PointerButtonReleased>(
                        &event.value()))
            {
                onPointerReleased(*releasedEvent);
                continue;
            }

            if (const auto *released =
                    std::get_if<input::KeyReleased>(&event.value()))
            {
                onKeyReleased(*released);
                continue;
            }

            if (const auto *pressedEvent =
                    std::get_if<input::KeyPressed>(&event.value()))
            {
                onKeyPressed(*pressedEvent);
            }
        }
    }

    void Editor::run()
    {
        lastFrameAt = clockSource.now();

        while (window->isOpen() && running)
        {
            const auto startedAt = clockSource.now();

            const auto since =
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    startedAt - lastFrameAt);

            meters.frameRate.record(since);
            lastFrameAt = startedAt;
            tickDebt += since;

            if (!pollWindow())
            {
                break;
            }

            pollInputs();

            if (!window->isOpen())
            {
                break;
            }

            frame(startedAt);
        }

        window->close();
        logger.log(log::Level::Info, "Antwika closed");
    }

}
