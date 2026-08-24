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

#include "antwika/editor/PreferencesFile.hpp"

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
              window->renderer(), window->getSize(), camera::kCanvasSize),
          play(logger, worldMeshes.getCells())
    {
        document.startFrom(std::move(mapPathGiven));
        document.map = map::Map{
            .voxels = voxel::getExpandCubesToVoxels(voxelmap::getDemoCells()),
            .tilemap = tilemap::getDefaultTilemap()};

        worldShader.open(viewportRenderer, map::getLoadShader("voxel"));
        keyBench.takeBindings(getLoadChords(getChordsPath()));

        try
        {
            document.map = map::getLoadMap(document.getPath());

            logger.log(log::Level::Info, "Loaded " + document.getPath());
        }
        catch (const map::MapFileError &)
        {
            logger.log(
                log::Level::Info,
                "No map at " + document.getPath()
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

        worldView.worldEdit.editLevel =
            antwika::voxel::getCubeIndexOfLevel(voxelmap::getTopLevel(
                    document.map.voxels));

        atlasSheets.open(
            viewportRenderer,
            map::getLoadAtlasPairOrBlank(document.getPath(), kAppName),
            document.map,
            tick);
        characterView.open(
            viewportRenderer, map::getLoadCharacterSheet(document.getPath(),
                kAppName));
        iconsView.open(
            viewportRenderer, getLoadIconSheet(document.getPath(), kAppName));
        sprites.open(viewportRenderer);
        lightPasses.open(viewportRenderer, map::getLoadShader("shadow"));
        scenePass.open(viewportRenderer, map::getLoadShader("bloom"));

        takePreferences(getLoadPreferences(document.getPath()));

        rebuildWorld();

        spawnRoster();
        loadCharacterSkins();

        const auto opening = map::CameraView{
            .transform = camera::getCenteredOn(camera::getDefaultTransform(),
                voxelmap::getVoxelsCenter(document.map.voxels))};

        cameraRig.view = document.map.camera.value_or(opening);
        cameraRig.viewHeight =
            camera::getOrthoHalfHeight(camera::kCanvasSize, cameraRig.view.zoom);

        if (playOnly)
        {
            const auto progress = map::getLoadProgress(getProgressPath());

            if (progress.has_value())
            {
                document.openAt((std::filesystem::path(document.getStartPath())
                               .parent_path()
                           / progress->map)
                              .string());

                if (loadCurrentMap())
                {
                    {
                        const ecs::OpenPhase phase(play.game->getWorld());

                        play.game->getWorld().set<component::Position>(
                            play.game->getPlayer(),
                            collision::positionFrom(
                                progress->stancePlacement.position));
                        play.game->getWorld().set<component::AnimationState>(
                            play.game->getPlayer(),
                            component::AnimationState{
                                .direction =
                                    progress->stancePlacement.way});
                    }

                    play.game->cameraTarget() =
                        progress->stancePlacement.position;
                }
                else
                {
                    document.openAt(document.getStartPath());
                }
            }

            play.playing = true;
            play.titleScreenUp = true;
            aimPlayCamera();
        }

        logger.log(log::Level::Info, "Antwika opened");
    }

    bool Editor::pollWindow()
    {
        const auto changes = app::windowChanges(backend, window->getId());

        if (changes.resizedSize.has_value())
        {
            viewportRenderer.resize(*changes.resizedSize);
        }

        const auto closeRequested = changes.closeRequested;

        if (closeRequested && document.isDirty())
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
        tickDebt.start();

        while (window->isOpen() && running)
        {
            meters.frameRate.record(tickDebt.advance());

            if (!pollWindow())
            {
                break;
            }

            pollInputs();

            if (!window->isOpen())
            {
                break;
            }

            frame(tickDebt.startedAt());
        }

        window->close();
        logger.log(log::Level::Info, "Antwika closed");
    }

}
