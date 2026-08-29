#include <utility>

#include <antwika/image/PngFile.hpp>
#include <antwika/component/AnimationState.hpp>
#include <antwika/component/Orientation.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/ecs/OpenPhase.hpp>
#include <antwika/decor/Decor.hpp>
#include <antwika/editor/ui/GizmoSheet.hpp>
#include <antwika/editor/ui/IconSheet.hpp>
#include <antwika/editor/ui/MapPicker.hpp>
#include <antwika/gfx/Camera3D.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/assets/MapAssets.hpp>
#include <antwika/map/MapFileError.hpp>
#include <antwika/map/MapWarnings.hpp>
#include <antwika/gameplay/Characters.hpp>
#include <antwika/rules/Orientation.hpp>
#include <antwika/solver/VoxelWeave.hpp>
#include <antwika/tilemap/AtlasLayout.hpp>
#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/voxel/VoxelCube.hpp>

#include "antwika/editor/PreferencesFile.hpp"

#include "antwika/editor/Editor.hpp"

namespace antwika::editor
{

    voxel::Voxels Editor::visibleCells()
    {
        if (!preferences.hideAboveLevel || play.playing)
        {
            return document.map.voxels;
        }

        voxel::Voxels keptVoxels;

        for (const auto &[position, material] : document.map.voxels)
        {
            if (position.y
                <= antwika::voxel::getCubeTop(
                       worldView.worldEdit().getEditLevel())
                       - voxel::kCubeSide)
            {
                keptVoxels[position] = material;
            }
        }

        return keptVoxels;
    }

    void Editor::rebuildWorld()
    {
        worldMeshes.rebuild(
            viewportRenderer,
            document.map,
            visibleCells(),
            worldView.worldEdit().getCornerJoining(),
            atlasSheets.getSheets(),
            tick);
        lightPasses.forget();
        worldView.markOverlaysStale();
    }

    component::Position Editor::playerStandsAt() const
    {
        return play.game->getWorld().get<component::Position>(
            play.game->getPlayer());
    }

    map::Placement Editor::startingPlacement()
    {
        return gameplay::getStartingPlacement(
            play.game->getWorld(),
            document.map,
            worldMeshes.getCells(),
            play.playing ? play.game->getCheckpoint().placement
                    : std::optional<map::Placement>{});
    }

    void Editor::standPlayer()
    {
        spawnCharacters();

        const auto startPlacement = startingPlacement();
        auto stoodPosition = collision::positionFrom(startPlacement.position);
        const auto ground = collision::getGroundHeightUnderFootprint(
            worldMeshes.getCells(), stoodPosition.x, stoodPosition.z,
            stoodPosition.y);

        if (ground.has_value())
        {
            stoodPosition.y = *ground;
        }
        else
        {
            const auto restPosition = collision::getRestPositionOverColumn(
                worldMeshes.getCells(),
                static_cast<std::int32_t>(
                    std::lround(stoodPosition.x)),
                static_cast<std::int32_t>(
                    std::lround(stoodPosition.z)));
            const auto put = restPosition.has_value()
                           ? restPosition
                           : collision::getSpawnPosition(document.map.voxels);

            if (put.has_value())
            {
                stoodPosition = *put;
            }
        }

        {
            const ecs::OpenPhase phase(play.game->getWorld());

            play.game->getWorld().set<component::Position>(
                play.game->getPlayer(), stoodPosition);
            play.game->getWorld().set<component::AnimationState>(
                play.game->getPlayer(),
                component::AnimationState{.direction = startPlacement.way});
        }

        play.game->setCameraTarget(
            antwika::gfx::Vec3{stoodPosition.x, stoodPosition.y,
            stoodPosition.z});
        play.game->clearPath();
        play.game->clearSteering();
    }

    void Editor::standPlayerAt(
        const std::int32_t x, const std::int32_t z)
    {
        standPlayer();

        const auto restPosition = collision::getRestPositionOverColumn(
            worldMeshes.getCells(), x, z);

        if (restPosition.has_value())
        {
            {
                const ecs::OpenPhase phase(play.game->getWorld());

                play.game->getWorld().set<component::Position>(
                    play.game->getPlayer(), *restPosition);
            }

            play.game->setCameraTarget(antwika::gfx::Vec3{
                restPosition->x, restPosition->y, restPosition->z});
        }
    }

    void Editor::aimPlayCamera()
    {
        const auto stoodPosition =
            play.game->getWorld().get<component::Position>(play.game->getPlayer());

        play.game->getCameraTransform() =
            camera::getSnappedPitch(camera::getDefaultTransform());
        play.game->aimAt(
            getWorldRotation(play),
            antwika::gfx::Vec3{stoodPosition.x, stoodPosition.y,
            stoodPosition.z});
    }

    void Editor::moveCamera()
    {
        const auto goal = camera::getOrthoHalfHeight(
            camera::kCanvasSize,
            play.playing ? play.game->getZoom() : cameraRig.view.zoom);

        cameraRig.viewHeight =
            std::abs(goal - cameraRig.viewHeight) < 0.001F
                ? goal
                : cameraRig.viewHeight
                      + ((goal - cameraRig.viewHeight)
                         * camera::kZoomLerpRate);

        if (!cameraRig.orbitFromPosition.has_value()
            || !isWorldShown()
            || play.playing || focusedField != FocusedField::Nothing
            || fileChooser.fileDialog.has_value() || dialogs.quitConfirmOpen
            || keyBench.panelShown
            || keyBench.rebindingAction.has_value() || getHeldModifiers().control
            || getHeldModifiers().alt)
        {
            return;
        }

        const auto byX = std::clamp(
            play.wasdKeys.getAxisX() + play.arrowKeys.getAxisX(),
            -1.0F, 1.0F);
        const auto byY = std::clamp(
            play.wasdKeys.getAxisZ() + play.arrowKeys.getAxisZ(),
            -1.0F, 1.0F);
        const auto byRise = worldView.worldEdit().getRiseAxis();

        if (byX != 0.0F || byY != 0.0F || byRise != 0.0F)
        {
            cameraRig.view.transform = camera::getMovedAlongView(
                cameraRig.view.transform,
                -byY,
                byX,
                byRise,
                getHeldModifiers().shift
                    ? camera::kFlyStep
                          * camera::kFlyBoost
                    : camera::kFlyStep);
        }
    }

    void Editor::turnPlayer(
        const float byYaw, const float byPitch)
    {
        play.game->getWorld().set<component::Orientation>(
            play.game->getEye(),
            antwika::rules::getTurnedBy(
                play.game->getWorld().get<component::Orientation>(play.game->getEye(
                        )),
                byYaw,
                byPitch));

        play.game->aimAt(getWorldRotation(play), play.game->getCameraTarget());
    }

    void Editor::saveCurrentMap()
    {
        if (document.getPath().empty())
        {
            dialogs.quitConfirmOpen = false;
            fileChooser.open(
                document.getPath(), document.getStartPath(), true);

            return;
        }

        try
        {
            document.map.camera = cameraRig.view;
            const auto hero = getPlayerIndex(document.map);

            if (hero.has_value())
            {
                document.map.characters.at(
                    *hero).idlePlacement = map::Placement{
                    .position = collision::positionOf(
                        play.game->getWorld().get<component::Position>(
                            play.game->getPlayer())),
                    .way = play.game->getWorld()
                               .get<component::AnimationState>(
                                   play.game->getPlayer())
                               .direction};
            }
            document.map.settings.cornersJoined =
                worldView.worldEdit().isCornerJoiningOn();

            auto keptMap = document.map;

            keptMap.decor = getCompactedDecor(document.map.decor);
            map::saveMap(document.getPath(), keptMap);
            savePreferences(document.getPath(), getPreferencesAsShown());

            for (std::size_t sheet = 0;
                 sheet < assets::kAtlasSheetCount;
                 ++sheet)
            {
                image::writePngFile(
                    atlasSheets.sheet(sheet),
                    map::getSidecarPath(
                        document.getPath(),
                        assets::kAtlasSheets.at(sheet).name),
                    kAppName);
            }
            saveCharacterSkins();

            if (iconsView.isUnsaved())
            {
                assets::writeSharedTexture(
                    iconsView.getSheet(),
                    document.getPath(),
                    antwika::editor::kIconSheet,
                    kAppName);
                iconsView.keep();
            }

            if (gizmos.unsaved)
            {
                assets::writeSharedTexture(
                    gizmos.sheetBitmap,
                    document.getPath(),
                    antwika::editor::kGizmoSheet,
                    kAppName);
                gizmos.unsaved = false;
            }

            if (const auto notice = plan.save(); notice.has_value())
            {
                showStatus(*notice, true, 600);
                logger.log(log::Level::Warning, *notice);
            }
            document.markSaved();
            showStatus("saved", false, 120);
            logger.log(
                log::Level::Info,
                "Saved " + document.getPath() + " and both atlases");
        }
        catch (const map::MapFileError &error)
        {
            showStatus(error.what(), true, 600);
            logger.log(log::Level::Warning, error.what());
        }
    }

    bool Editor::loadCurrentMap()
    {
        try
        {
            document.map = map::getLoadMap(document.getPath());
            document.forgetHistory();

            const auto camera = document.map.camera.value_or(
                map::CameraView{
                    .transform = camera::getCenteredOn(
                        cameraRig.view.transform,
                        voxelmap::getVoxelsCenter(document.map.voxels)),
                    .zoom = cameraRig.view.zoom});

            cameraRig.view = camera;
            cameraRig.viewHeight =
                camera::getOrthoHalfHeight(camera::kCanvasSize,
                    cameraRig.view.zoom);
            Preferences restingPreferences;

            restingPreferences.lighting = document.map.settings.lighting;

            takePreferences(
                getLoadPreferences(document.getPath(), restingPreferences));
            atlasSheets.take(
                assets::getLoadAtlasPairOrBlank(document.getPath(), kAppName));
            iconsView.open(
                viewportRenderer, getLoadIconSheet(document.getPath(), kAppName));
            openGizmoSheet();
            characterView.editFirst();
            worldView.characterTool().dropChoice();
            worldView.worldEdit().setEditLevel(
                antwika::voxel::getCubeIndexOfLevel(
                    voxelmap::getTopLevel(document.map.voxels)));
            rebuildWorld();
            play.game->setCheckpoint(gameplay::CheckpointState{});
            standPlayer();
            loadCharacterSkins();
            document.markSaved();
            logger.log(
                log::Level::Info,
                "Loaded " + document.getPath() + ": "
                    + solver::getWeaveErrorMessage(
                        worldMeshes.getFaces(),
                        worldMeshes.getRules(),
                        worldMeshes.getWeaveSolve(),
                        worldView.worldEdit().getCornerJoining()));

            const auto warnings = map::getMapWarnings(document.map);

            for (const auto &warning : warnings)
            {
                logger.log(log::Level::Warning, warning);
            }

            if (!warnings.empty())
            {
                showStatus(warnings.front(), true, 600);
            }
        }
        catch (const map::MapFileError &error)
        {
            showStatus(error.what(), true, 600);
            logger.log(log::Level::Warning, error.what());

            return false;
        }

        return true;
    }

    void Editor::startNewMap()
    {
        pushUndo();

        map::Map emptyMap;

        document.map = std::move(emptyMap);
        document.openAt({});
        atlasSheets.take(
            {tilemap::getBlankAtlas(tilemap::kWallTileSize),
             tilemap::getBlankAtlas(tilemap::kFloorTileSize)});
        atlasSheets.touch();
        characterView.editFirst();
        worldView.characterTool().dropChoice();
        worldView.worldEdit().setEditLevel(
            antwika::voxel::getCubeIndexOfLevel(voxelmap::getTopLevel(
                    document.map.voxels)));
        rebuildWorld();
        play.game->setCheckpoint(gameplay::CheckpointState{});
        spawnCharacters();
        standPlayer();
        loadCharacterSkins();
        document.markDirty();
    }

    void Editor::confirmFileDialog()
    {
        const auto choice = fileChooser.confirm(*this);

        if (!choice.has_value())
        {
            return;
        }

        document.openAt(choice->path);

        if (choice->isSaveMode)
        {
            saveCurrentMap();
        }
        else
        {
            loadCurrentMap();
        }
    }

    void Editor::takePreferences(const Preferences &shownPreferences)
    {
        preferences = shownPreferences;
        viewChoice.activeView = shownPreferences.view;
        worldView.worldEdit().setCornerJoining(
            document.map.settings.cornersJoined);
    }

    Preferences Editor::getPreferencesAsShown() const
    {
        auto shownPreferences = preferences;

        shownPreferences.view = viewChoice.activeView;

        return shownPreferences;
    } // GCOVR_EXCL_LINE

    Snapshot Editor::snapshot()
    {
        return Snapshot{
            .map = document.map,
            .pixelBitmaps = atlasSheets.getSheets(),
            .characterBitmaps = characterView.getSkinsAsDrawn(characterSkins)};
    } // GCOVR_EXCL_LINE

    void Editor::pushUndo()
    {
        if (play.playing)
        {
            return;
        }

        document.push(snapshot());
        document.markDirty();
    }

    void Editor::undo()
    {
        settleHistory(
            document.undo(snapshot()),
            "undone",
            "nothing to undo");
    }

    void Editor::redo()
    {
        settleHistory(
            document.redo(snapshot()),
            "redone",
            "nothing to redo");
    }

    void Editor::settleHistory(
        std::optional<Snapshot> stepSnapshot,
        const std::string &doneLabel,
        const std::string &nothing)
    {
        if (stepSnapshot.has_value())
        {
            applyStep(std::move(*stepSnapshot));
            showStatus(doneLabel, false, 120);

            return;
        }

        showStatus(nothing, false, 120);
    }

    void Editor::applyStep(Snapshot stepSnapshot)
    {
        worldView.clearGrowTrouble();
        document.map = std::move(stepSnapshot.map);
        atlasSheets.take(std::move(stepSnapshot.pixelBitmaps));
        characterView.takeSkins(
            viewportRenderer,
            characterSkins, std::move(stepSnapshot.characterBitmaps));
        rebuildWorld();
        spawnCharacters();
        atlasSheets.touch();
        characterView.touch();
    }

}
