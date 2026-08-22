#include <utility>

#include <antwika/gfx/PngFile.hpp>
#include <antwika/component/AnimationState.hpp>
#include <antwika/component/Orientation.hpp>
#include <antwika/component/Position.hpp>
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

#include "antwika/editor/Editor.hpp"

namespace antwika::editor
{

    voxel::Voxels Editor::visibleCells()
    {
        if (!hideAboveLevel || playing)
        {
            return map.voxels;
        }

        voxel::Voxels keptVoxels;

        for (const auto &[position, material] : map.voxels)
        {
            if (position.y
                <= antwika::voxel::cubeTop(editLevel) - voxel::kCubeSide)
            {
                keptVoxels[position] = material;
            }
        }

        return keptVoxels;
    }

    void Editor::rebuildDecorMesh()
    {
        worldMeshes.rebuildDecor(viewportRenderer, map, tick);
    }

    void Editor::rebuildWorld()
    {
        worldMeshes.rebuild(
            viewportRenderer,
            map,
            visibleCells(),
            cornerJoining,
            atlasSheets.sheets(),
            tick);
        lightPasses.forget();
        overlayStale = true;
    }

    map::Placement Editor::startingPlacement()
    {
        return gameplay::startingPlacement(
            map,
            worldMeshes.cells(),
            playing ? game->gates().checkpointPlacement
                    : std::optional<map::Placement>{});
    }

    void Editor::standPlayer()
    {
        const auto startPlacement = startingPlacement();
        auto stoodPosition = collision::positionFrom(startPlacement.position);
        const auto ground = collision::groundHeightUnderFootprint(
            worldMeshes.cells(), stoodPosition.x, stoodPosition.z,
            stoodPosition.y);

        if (ground.has_value())
        {
            stoodPosition.y = *ground;
        }
        else
        {
            const auto restPosition = collision::restPositionOverColumn(
                worldMeshes.cells(),
                static_cast<std::int32_t>(
                    std::lround(stoodPosition.x)),
                static_cast<std::int32_t>(
                    std::lround(stoodPosition.z)));
            const auto put = restPosition.has_value()
                           ? restPosition
                           : collision::spawnPosition(map.voxels);

            if (put.has_value())
            {
                stoodPosition = *put;
            }
        }

        spawnRoster();
        game->world().set<component::Position>(game->player(), stoodPosition);
        game->world().set<component::AnimationState>(
            game->player(),
            component::AnimationState{.direction = startPlacement.way});
        game->world().commit();

        game->cameraTarget() =
            antwika::gfx::Vec3{stoodPosition.x, stoodPosition.y,
            stoodPosition.z};
        game->clearPath();
        game->clearSteering();
    }

    void Editor::standPlayerAt(
        const std::int32_t x, const std::int32_t z)
    {
        standPlayer();

        const auto restPosition = collision::restPositionOverColumn(
            worldMeshes.cells(), x, z);

        if (restPosition.has_value())
        {
            game->world().set<component::Position>(
                game->player(), *restPosition);
            game->world().commit();
            game->cameraTarget() = antwika::gfx::Vec3{
                restPosition->x, restPosition->y, restPosition->z};
        }
    }

    gfx::Mat4 Editor::worldRotation()
    {
        const auto orientation =
            game->world().get<component::Orientation>(game->eye());

        return voxelmap::modelRotation(orientation.yaw, orientation.pitch);
    }

    gfx::Camera3D Editor::worldCamera()
    {
        return playing
                   ? camera::cameraOf(
                         game->cameraTransform(),
                         camera::kCanvasSize,
                         viewHeight)
                   : camera::perspectiveOf(
                       cameraView.transform,
                       camera::kCanvasSize,
                       viewHeight);
    }

    void Editor::aimPlayCamera()
    {
        const auto stoodPosition =
            game->world().get<component::Position>(game->player());

        game->cameraTransform() =
            camera::snappedPitch(camera::defaultTransform());
        game->aimAt(
            worldRotation(),
            antwika::gfx::Vec3{stoodPosition.x, stoodPosition.y,
            stoodPosition.z});
    }

    void Editor::moveCamera()
    {
        const auto goal = camera::orthoHalfHeight(
            camera::kCanvasSize, playing ? game->zoom() : cameraView.zoom);

        viewHeight =
            std::abs(goal - viewHeight) < 0.001F
                ? goal
                : viewHeight
                      + ((goal - viewHeight)
                         * camera::kZoomLerpRate);

        if (!orbitFromPosition.has_value()
            || activeView != map::View::World
            || playing || focusedField != FocusedField::Nothing
            || dialogs.fileDialog.has_value() || dialogs.quitConfirmOpen
            || keysOpen
            || rebindingAction.has_value() || heldModifiers().control
            || heldModifiers().alt)
        {
            return;
        }

        const auto byX = std::clamp(
            game->wasdKeys().axisX() + game->arrowKeys().axisX(), -1.0F, 1.0F);
        const auto byY = std::clamp(
            game->wasdKeys().axisZ() + game->arrowKeys().axisZ(), -1.0F, 1.0F);
        const auto byRise =
            (ascendHeld ? 1.0F : 0.0F)
            - (descendHeld ? 1.0F : 0.0F);

        if (byX != 0.0F || byY != 0.0F || byRise != 0.0F)
        {
            cameraView.transform = camera::movedAlongView(
                cameraView.transform,
                -byY,
                byX,
                byRise,
                heldModifiers().shift
                    ? camera::kFlyStep
                          * camera::kFlyBoost
                    : camera::kFlyStep);
        }
    }

    void Editor::turnPlayer(
        const float byYaw, const float byPitch)
    {
        game->world().set<component::Orientation>(
            game->eye(),
            antwika::rules::turnedBy(
                game->world().get<component::Orientation>(game->eye()),
                byYaw,
                byPitch));

        game->aimAt(worldRotation(), game->cameraTarget());
    }

    void Editor::orbitCamera(
        const float byYaw, const float byPitch)
    {
        const auto backDistance =
            viewHeight
            / std::tan(camera::kEditorFov / 2.0F);
        const auto eye =
            cameraView.transform.position
            - (camera::forward(cameraView.transform) * backDistance);

        cameraView.transform =
            camera::rotated(cameraView.transform, byYaw, byPitch);
        cameraView.transform.position =
            eye + (camera::forward(cameraView.transform) * backDistance);
    }

    void Editor::saveCurrentMap()
    {
        if (mapPath.empty())
        {
            dialogs.quitConfirmOpen = false;
            openFileDialog(true);

            return;
        }

        try
        {
            map.camera = cameraView;
            const auto hero = playerIndex(map);

            if (hero.has_value())
            {
                map.characters.at(*hero).idlePlacement = map::Placement{
                    .position = collision::positionOf(
                        game->world().get<component::Position>(game->player())),
                    .way = game->world()
                               .get<component::AnimationState>(game->player())
                               .direction};
            }
            map.settings = map::Settings{
                .lighting = lighting,
                .showRuleLines = showRuleLines,
                .tool = tool,
                .paint = paintMode,
                .view = playing ? viewBeforePlay : activeView,
                .kind = brushKind,
                .grid = grid,
                .showPlacementGhost = showPlacementGhost,
                .lampSight = lampSight,
                .cameraFollows = cameraFollows,
                .hideAboveLevel = hideAboveLevel,
                .cornersJoined =
                    cornerJoining == solver::CornerSeams::Included};

            gfx::writePngFile(
                atlasSheets.sheet(0),
                map::sidecarPath(mapPath, "atlas-15x9.png"),
                kAppName);
            gfx::writePngFile(
                atlasSheets.sheet(1),
                map::sidecarPath(mapPath, "atlas-15x12.png"),
                kAppName);
            saveCharacterSkins();

            if (iconsView.unsaved())
            {
                map::writeSharedTexture(
                    iconsView.sheet(),
                    mapPath,
                    antwika::editor::kIconSheet,
                    kAppName);
                iconsView.keep();
            }

            auto keptMap = map;

            keptMap.decor = compactedDecor(map.decor);
            map::saveMap(mapPath, keptMap);
            if (const auto notice = plan.save(); notice.has_value())
            {
                showStatus(*notice, true, 600);
                logger.log(log::Level::Warning, *notice);
            }
            dirty = false;
            showStatus("saved", false, 120);
            logger.log(
                log::Level::Info,
                "Saved " + mapPath + " and both atlases");
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
            map = map::loadMap(mapPath);
            history.clear();

            const auto camera = map.camera.value_or(
                map::CameraView{
                    .transform = camera::centeredOn(
                        cameraView.transform,
                        voxelmap::voxelsCenter(map.voxels)),
                    .zoom = cameraView.zoom});

            cameraView = camera;
            viewHeight =
                camera::orthoHalfHeight(camera::kCanvasSize, cameraView.zoom);
            lighting = map.settings.lighting;
            showRuleLines = map.settings.showRuleLines;
            tool = map.settings.tool;
            paintMode = map.settings.paint;
            activeView = map.settings.view;
            brushKind = map.settings.kind;
            grid = map.settings.grid;
            showPlacementGhost = map.settings.showPlacementGhost;
            lampSight = map.settings.lampSight;
            cameraFollows = map.settings.cameraFollows;
            hideAboveLevel = map.settings.hideAboveLevel;
            cornerJoining = map.settings.cornersJoined
                          ? solver::CornerSeams::Included
                          : solver::CornerSeams::Ignored;
            atlasSheets.take(
                {map::loadAtlasOrBlank(
                     mapPath,
                     "atlas-15x9.png",
                     tilemap::kWallTileSize,
                     kAppName),
                 map::loadAtlasOrBlank(
                     mapPath,
                     "atlas-15x12.png",
                     tilemap::kFloorTileSize,
                     kAppName)});
            iconsView.open(
                viewportRenderer, loadIconSheet(mapPath, kAppName));
            characterView.editFirst();
            figurePicked.reset();
            editLevel =
                antwika::voxel::cubeIndexOfLevel(
                    voxelmap::topLevel(map.voxels));
            rebuildWorld();
            resetGates();
            standPlayer();
            loadCharacterSkins();
            dirty = false;
            logger.log(
                log::Level::Info,
                "Loaded " + mapPath + ": "
                    + solver::weaveErrorMessage(
                        voxelmap::visibleFacesOf(map.voxels),
                        map.rules,
                        solver::solveTiles(
                            voxelmap::visibleFacesOf(map.voxels),
                            map.rules,
                            cornerJoining),
                        cornerJoining));
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

        map = std::move(emptyMap);
        mapPath.clear();
        atlasSheets.take(
            {tilemap::blankAtlas(tilemap::kWallTileSize),
             tilemap::blankAtlas(tilemap::kFloorTileSize)});
        atlasSheets.touch();
        characterView.editFirst();
        figurePicked.reset();
        editLevel =
            antwika::voxel::cubeIndexOfLevel(voxelmap::topLevel(map.voxels));
        rebuildWorld();
        resetGates();
        spawnRoster();
        standPlayer();
        loadCharacterSkins();
        dirty = true;
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

        dialogs.mapEntries = antwika::editor::filterMapNames(names);
    }

    void Editor::openFileDialog(const bool forSave)
    {
        const auto path = std::filesystem::absolute(
            mapPath.empty() ? startMapPath : mapPath);
        const auto folder = path.parent_path().string();

        dialogs.fileDialog = FileDialog{
            .isSaveMode = forSave,
            .folder = folder,
            .fileName =
                mapPath.empty() ? std::string{} : path.filename().string()};
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
             / antwika::editor::ensureMapExtension(
                 dialogs.fileDialog->fileName))
                .string();

        dialogs.fileDialog.reset();
        mapPath = path;

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

    map::Snapshot Editor::snapshot()
    {
        return map::Snapshot{
            .map = map,
            .pixelBitmaps = atlasSheets.sheets(),
            .characterBitmaps = characterView.skinsAsDrawn()};
    } // GCOVR_EXCL_LINE

    void Editor::pushUndo()
    {
        history.push(snapshot());
        dirty = true;
    }

    void Editor::undo()
    {
        settleHistory(
            history.undo(snapshot()),
            "undone",
            "nothing to undo");
    }

    void Editor::redo()
    {
        settleHistory(
            history.redo(snapshot()),
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
        growTroublePositions.clear();
        map = std::move(stepSnapshot.map);
        atlasSheets.take(std::move(stepSnapshot.pixelBitmaps));
        characterView.takeSkins(
            viewportRenderer, std::move(stepSnapshot.characterBitmaps));
        rebuildWorld();
        spawnRoster();
        atlasSheets.touch();
        characterView.touch();
    }

}
