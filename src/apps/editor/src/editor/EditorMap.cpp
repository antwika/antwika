#include <utility>

#include <antwika/image/PngFile.hpp>
#include <antwika/component/AnimationState.hpp>
#include <antwika/component/Orientation.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/ecs/OpenPhase.hpp>
#include <antwika/decor/Decor.hpp>
#include <antwika/editor/ui/IconSheet.hpp>
#include <antwika/editor/ui/MapPicker.hpp>
#include <antwika/gfx/Camera3D.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/map/MapAssets.hpp>
#include <antwika/map/MapFileError.hpp>
#include <antwika/gameplay/Roster.hpp>
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
                <= antwika::voxel::getCubeTop(worldView.worldEdit.editLevel) - voxel::kCubeSide)
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
            worldView.worldEdit.cornerJoining,
            atlasSheets.getSheets(),
            tick);
        lightPasses.forget();
        worldView.overlays.stale = true;
    }

    component::Position Editor::playerStandsAt() const
    {
        return play.game->getWorld().get<component::Position>(
            play.game->getPlayer());
    }

    map::Placement Editor::startingPlacement()
    {
        return gameplay::getStartingPlacement(
            document.map,
            worldMeshes.getCells(),
            play.playing ? play.game->getGates().checkpointPlacement
                    : std::optional<map::Placement>{});
    }

    void Editor::standPlayer()
    {
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

        spawnRoster();

        {
            const ecs::OpenPhase phase(play.game->getWorld());

            play.game->getWorld().set<component::Position>(
                play.game->getPlayer(), stoodPosition);
            play.game->getWorld().set<component::AnimationState>(
                play.game->getPlayer(),
                component::AnimationState{.direction = startPlacement.way});
        }

        play.game->cameraTarget() =
            antwika::gfx::Vec3{stoodPosition.x, stoodPosition.y,
            stoodPosition.z};
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

            play.game->cameraTarget() = antwika::gfx::Vec3{
                restPosition->x, restPosition->y, restPosition->z};
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
            play.playing ? play.game->zoom() : cameraRig.view.zoom);

        cameraRig.viewHeight =
            std::abs(goal - cameraRig.viewHeight) < 0.001F
                ? goal
                : cameraRig.viewHeight
                      + ((goal - cameraRig.viewHeight)
                         * camera::kZoomLerpRate);

        if (!cameraRig.orbitFromPosition.has_value()
            || !isWorldShown()
            || play.playing || focusedField != FocusedField::Nothing
            || dialogs.fileDialog.has_value() || dialogs.quitConfirmOpen
            || keyBench.panelShown
            || keyBench.rebindingAction.has_value() || getHeldModifiers().control
            || getHeldModifiers().alt)
        {
            return;
        }

        const auto byX = std::clamp(
            play.game->wasdKeys().getAxisX() + play.game->arrowKeys().getAxisX(),
            -1.0F, 1.0F);
        const auto byY = std::clamp(
            play.game->wasdKeys().getAxisZ() + play.game->arrowKeys().getAxisZ(),
            -1.0F, 1.0F);
        const auto byRise =
            (worldView.worldEdit.ascendHeld ? 1.0F : 0.0F)
            - (worldView.worldEdit.descendHeld ? 1.0F : 0.0F);

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

        play.game->aimAt(getWorldRotation(play), play.game->cameraTarget());
    }

    void Editor::saveCurrentMap()
    {
        if (document.getPath().empty())
        {
            dialogs.quitConfirmOpen = false;
            openFileDialog(true);

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
                worldView.worldEdit.cornerJoining == solver::CornerSeams::Included;

            savePreferences(document.getPath(), getPreferencesAsShown());

            for (std::size_t sheet = 0;
                 sheet < map::kAtlasSheetCount;
                 ++sheet)
            {
                image::writePngFile(
                    atlasSheets.sheet(sheet),
                    map::getSidecarPath(
                        document.getPath(),
                        map::kAtlasSheets.at(sheet).name),
                    kAppName);
            }
            saveCharacterSkins();

            if (iconsView.isUnsaved())
            {
                map::writeSharedTexture(
                    iconsView.getSheet(),
                    document.getPath(),
                    antwika::editor::kIconSheet,
                    kAppName);
                iconsView.keep();
            }

            auto keptMap = document.map;

            keptMap.decor = getCompactedDecor(document.map.decor);
            map::saveMap(document.getPath(), keptMap);
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
            takePreferences(getLoadPreferences(document.getPath()));
            atlasSheets.take(
                map::getLoadAtlasPairOrBlank(document.getPath(), kAppName));
            iconsView.open(
                viewportRenderer, getLoadIconSheet(document.getPath(), kAppName));
            characterView.editFirst();
            worldView.figureTool.chosenIndex.reset();
            worldView.worldEdit.editLevel =
                antwika::voxel::getCubeIndexOfLevel(
                    voxelmap::getTopLevel(document.map.voxels));
            rebuildWorld();
            resetGates();
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
                        worldView.worldEdit.cornerJoining));
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
        worldView.figureTool.chosenIndex.reset();
        worldView.worldEdit.editLevel =
            antwika::voxel::getCubeIndexOfLevel(voxelmap::getTopLevel(
                    document.map.voxels));
        rebuildWorld();
        resetGates();
        spawnRoster();
        standPlayer();
        loadCharacterSkins();
        document.markDirty();
    }

    void Editor::listFolder(const std::string &folder)
    {
        std::vector<std::string> names;

        dialogs.folderEntries.clear();
        dialogs.mapEntries.clear();

        try
        {
            for (const auto &entry :
                 std::filesystem::directory_iterator(folder))
            {
                const auto tail =
                    entry.path().filename().string();

                if (entry.is_directory())
                {
                    dialogs.folderEntries.push_back(tail);
                }
                else
                {
                    names.push_back(tail);
                }
            }
        }
        catch (const std::filesystem::filesystem_error &)
        {
            return;
        }

        std::sort(dialogs.folderEntries.begin(), dialogs.folderEntries.end());

        if (dialogs.folderEntries.size()
            > antwika::editor::kMaxPicked)
        {
            dialogs.folderEntries.resize(antwika::editor::kMaxPicked);
        }

        dialogs.mapEntries = antwika::editor::getFilterMapNames(names);
    }

    void Editor::openFileDialog(const bool forSave)
    {
        const auto path = std::filesystem::absolute(
            document.getPath().empty() ? document.getStartPath() : document.getPath());
        const auto folder = path.parent_path().string();

        dialogs.fileDialog = FileDialog{
            .isSaveMode = forSave,
            .folder = folder,
            .fileName =
                document.getPath().empty() ? std::string{} : path.filename(
                    ).string()};
        listFolder(folder);
    }

    void Editor::confirmFileDialog()
    {
        if (!dialogs.fileDialog.has_value())
        {
            return;
        }

        if (dialogs.fileDialog->fileName.empty())
        {
            showStatus("the map needs a name", true, 180);

            return;
        }

        const auto forSave = dialogs.fileDialog->isSaveMode;
        const auto path =
            (std::filesystem::path(dialogs.fileDialog->folder)
             / antwika::editor::getEnsureMapExtension(
                 dialogs.fileDialog->fileName))
                .string();

        dialogs.fileDialog.reset();
        document.openAt(path);

        if (forSave)
        {
            saveCurrentMap();
        }
        else
        {
            loadCurrentMap();
        }
    }

    void Editor::cancelFileDialog()
    {
        dialogs.fileDialog.reset();
    }

    void Editor::takePreferences(const Preferences &shownPreferences)
    {
        preferences = shownPreferences;
        viewChoice.activeView = shownPreferences.view;
        worldView.worldEdit.cornerJoining = document.map.settings.cornersJoined
                      ? solver::CornerSeams::Included
                      : solver::CornerSeams::Ignored;
    }

    Preferences Editor::getPreferencesAsShown() const
    {
        auto shownPreferences = preferences;

        shownPreferences.view = viewChoice.activeView;

        return shownPreferences;
    } // GCOVR_EXCL_LINE

    map::Snapshot Editor::snapshot()
    {
        return map::Snapshot{
            .map = document.map,
            .pixelBitmaps = atlasSheets.getSheets(),
            .characterBitmaps = characterView.getSkinsAsDrawn(rosterSkins)};
    } // GCOVR_EXCL_LINE

    void Editor::pushUndo()
    {
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
        std::optional<map::Snapshot> stepSnapshot,
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

    void Editor::applyStep(map::Snapshot stepSnapshot)
    {
        worldView.grow.troublePositions.clear();
        document.map = std::move(stepSnapshot.map);
        atlasSheets.take(std::move(stepSnapshot.pixelBitmaps));
        characterView.takeSkins(
            viewportRenderer,
            rosterSkins, std::move(stepSnapshot.characterBitmaps));
        rebuildWorld();
        spawnRoster();
        atlasSheets.touch();
        characterView.touch();
    }

}
