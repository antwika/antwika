#include <cmath>
#include <numbers>
#include <variant>

#include <antwika/component/AnimationState.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/ecs/OpenPhase.hpp>
#include <antwika/editor/ui/GizmoSheet.hpp>
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
#include <antwika/assets/MapAssets.hpp>
#include <antwika/assets/ShaderAssets.hpp>
#include <antwika/map/MapFileError.hpp>
#include <antwika/render/Checkerboard.hpp>
#include <antwika/solver/VoxelWeave.hpp>
#include <antwika/tilemap/Tilemap.hpp>
#include <antwika/ui/Overloaded.hpp>
#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/voxelmap/VoxelPick.hpp>
#include <antwika/app/FramePacing.hpp>
#include <antwika/app/WindowEvents.hpp>

#include "antwika/editor/PreferencesFile.hpp"

#include "antwika/editor/Editor.hpp"
#include "antwika/editor/plan/PlanFileError.hpp"

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
                      .title = "Antwika game editor",
                      .size = app::kDefaultWindowSize,
                      .resizable = true,
                      .targetFps = app::kTargetFps})),
          viewportRenderer(
              window->renderer(), window->getSize(), camera::kCanvasSize),
          play(logger, document.map, worldMeshes.getCells())
    {
        document.startFrom(std::move(mapPathGiven));

        worldShader.open(viewportRenderer, assets::getShaderSource("voxel"));
        keyBench.takeBindings(getLoadChords(getChordsPath()));

        loadMapOrBuiltIn();
        loadPlan(std::move(planPathGiven));
        openSheets();
        openPasses();

        Preferences restingPreferences;

        restingPreferences.lighting = document.map.settings.lighting;

        takePreferences(
            getLoadPreferences(document.getPath(), restingPreferences));

        rebuildWorld();

        spawnCharacters();
        loadCharacterSkins();

        aimOpeningCamera();

        if (playOnly)
        {
            beginPlay();
        }

        logger.log(log::Level::Info, "Antwika opened");
    }

    void Editor::loadMapOrBuiltIn()
    {
        try
        {
            document.map = map::getLoadMap(document.getPath());

            logger.log(log::Level::Info, "Loaded " + document.getPath());
        }
        catch (const map::MapFileError &)
        {
            document.map = map::Map{
                .voxels =
                    voxel::getExpandCubesToVoxels(voxelmap::getDemoCells()),
                .tilemap = tilemap::getDefaultTilemap()};

            logger.log(
                log::Level::Info,
                "No map at " + document.getPath()
                    + ", starting from the built-in one");
        }

        worldView.worldEdit().setEditLevel(
            antwika::voxel::getCubeIndexOfLevel(voxelmap::getTopLevel(
                    document.map.voxels)));
    }

    void Editor::loadPlan(std::string planPathGiven)
    {
        try
        {
            plan.open(std::move(planPathGiven));
        }
        catch (const PlanFileError &error)
        {
            logger.log(log::Level::Warning, error.what());
        }
    }

    void Editor::openSheets()
    {
        atlasSheets.open(
            viewportRenderer,
            assets::getLoadAtlasPairOrBlank(document.getPath(), kAppName),
            document.map,
            tick);
        characterView.open(
            viewportRenderer, assets::getLoadCharacterSheet(document.getPath(),
                kAppName));
        iconsView.open(
            viewportRenderer, getLoadIconSheet(document.getPath(), kAppName));
        openGizmoSheet();
    }

    void Editor::openGizmoSheet()
    {
        gizmos.sheetBitmap = getLoadGizmoSheet(document.getPath(), kAppName);
        gizmos.texture = viewportRenderer.createTexture(gizmos.sheetBitmap);
        gizmos.unsaved = false;
    }

    void Editor::openPasses()
    {
        sprites.open(viewportRenderer);
        lightPasses.open(viewportRenderer, assets::getShaderSource("shadow"));
        scenePass.open(viewportRenderer, assets::getShaderSource("bloom"));
    }

    void Editor::aimOpeningCamera()
    {
        const auto opening = map::CameraView{
            .transform = camera::getCenteredOn(camera::getDefaultTransform(),
                voxelmap::getVoxelsCenter(document.map.voxels))};

        cameraRig.view = document.map.camera.value_or(opening);
        cameraRig.viewHeight =
            camera::getOrthoHalfHeight(camera::kCanvasSize, cameraRig.view.zoom);
    }

    void Editor::beginPlay()
    {
        restoreProgress();

        keepMapForPlay();
        play.playing = true;
        play.titleScreenUp = true;
        aimPlayCamera();
    }

    void Editor::keepMapForPlay()
    {
        play.mapBeforePlay = document.map;
        play.wasDirtyBeforePlay = document.isDirty();
    }

    void Editor::restoreMapAfterPlay()
    {
        if (!play.mapBeforePlay.has_value())
        {
            if (preferences.hideAboveLevel)
            {
                rebuildWorld();
            }

            return;
        }

        document.map = std::move(*play.mapBeforePlay);
        play.mapBeforePlay.reset();
        rebuildWorld();

        if (!play.wasDirtyBeforePlay)
        {
            document.markSaved();
        }
    }

    void Editor::restoreProgress()
    {
        const auto progress = map::getLoadProgress(getProgressPath());

        if (!progress.has_value())
        {
            return;
        }

        document.openAt(document.getStartSiblingPath(progress->map));

        if (!loadCurrentMap())
        {
            document.openAt(document.getStartPath());

            return;
        }

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

        play.game->setCameraTarget(progress->stancePlacement.position);
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

            std::visit(
                ui::Overloaded{
                    [this](const input::PointerScrolled &rolledScrolled)
                    {
                        onScrolled(rolledScrolled);
                    },
                    [this](const input::PointerMoved &movedEvent)
                    {
                        onPointerMoved(movedEvent);
                    },
                    [this](const input::PointerButtonPressed &downPressed)
                    {
                        onPointerPressed(downPressed);
                    },
                    [this](const input::PointerButtonReleased &upReleased)
                    {
                        onPointerReleased(upReleased);
                    },
                    [this](const input::KeyReleased &releasedEvent)
                    {
                        onKeyReleased(releasedEvent);
                    },
                    [this](const input::KeyPressed &pressedKey)
                    {
                        onKeyPressed(pressedKey);
                    }},
                *event);
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
