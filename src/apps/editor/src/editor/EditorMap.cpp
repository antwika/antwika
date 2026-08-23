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

#include "antwika/editor/Editor.hpp"

namespace antwika::editor
{

    voxel::Voxels Editor::visibleCells()
    {
        if (!settings.hideAboveLevel || play.playing)
        {
            return document.map.voxels;
        }

        voxel::Voxels keptVoxels;

        for (const auto &[position, material] : document.map.voxels)
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
        worldMeshes.rebuildDecor(viewportRenderer, document.map, tick);
    }

    void Editor::rebuildWorld()
    {
        worldMeshes.rebuild(
            viewportRenderer,
            document.map,
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
            document.map,
            worldMeshes.cells(),
            play.playing ? play.game->gates().checkpointPlacement
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
                           : collision::spawnPosition(document.map.voxels);

            if (put.has_value())
            {
                stoodPosition = *put;
            }
        }

        spawnRoster();

        {
            const ecs::OpenPhase phase(play.game->world());

            play.game->world().set<component::Position>(
                play.game->player(), stoodPosition);
            play.game->world().set<component::AnimationState>(
                play.game->player(),
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

        const auto restPosition = collision::restPositionOverColumn(
            worldMeshes.cells(), x, z);

        if (restPosition.has_value())
        {
            {
                const ecs::OpenPhase phase(play.game->world());

                play.game->world().set<component::Position>(
                    play.game->player(), *restPosition);
            }

            play.game->cameraTarget() = antwika::gfx::Vec3{
                restPosition->x, restPosition->y, restPosition->z};
        }
    }

    gfx::Mat4 Editor::worldRotation()
    {
        const auto orientation =
            play.game->world().get<component::Orientation>(play.game->eye());

        return voxelmap::modelRotation(orientation.yaw, orientation.pitch);
    }

    gfx::Camera3D Editor::worldCamera()
    {
        return play.playing
                   ? camera::cameraOf(
                         play.game->cameraTransform(),
                         camera::kCanvasSize,
                         cameraRig.viewHeight)
                   : camera::perspectiveOf(
                       cameraRig.view.transform,
                       camera::kCanvasSize,
                       cameraRig.viewHeight);
    }

    void Editor::aimPlayCamera()
    {
        const auto stoodPosition =
            play.game->world().get<component::Position>(play.game->player());

        play.game->cameraTransform() =
            camera::snappedPitch(camera::defaultTransform());
        play.game->aimAt(
            worldRotation(),
            antwika::gfx::Vec3{stoodPosition.x, stoodPosition.y,
            stoodPosition.z});
    }

    void Editor::moveCamera()
    {
        const auto goal = camera::orthoHalfHeight(
            camera::kCanvasSize,
            play.playing ? play.game->zoom() : cameraRig.view.zoom);

        cameraRig.viewHeight =
            std::abs(goal - cameraRig.viewHeight) < 0.001F
                ? goal
                : cameraRig.viewHeight
                      + ((goal - cameraRig.viewHeight)
                         * camera::kZoomLerpRate);

        if (!cameraRig.orbitFromPosition.has_value()
            || activeView != map::View::World
            || play.playing || focusedField != FocusedField::Nothing
            || dialogs.fileDialog.has_value() || dialogs.quitConfirmOpen
            || keysOpen
            || rebindingAction.has_value() || heldModifiers().control
            || heldModifiers().alt)
        {
            return;
        }

        const auto byX = std::clamp(
            play.game->wasdKeys().axisX() + play.game->arrowKeys().axisX(),
            -1.0F, 1.0F);
        const auto byY = std::clamp(
            play.game->wasdKeys().axisZ() + play.game->arrowKeys().axisZ(),
            -1.0F, 1.0F);
        const auto byRise =
            (ascendHeld ? 1.0F : 0.0F)
            - (descendHeld ? 1.0F : 0.0F);

        if (byX != 0.0F || byY != 0.0F || byRise != 0.0F)
        {
            cameraRig.view.transform = camera::movedAlongView(
                cameraRig.view.transform,
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
        play.game->world().set<component::Orientation>(
            play.game->eye(),
            antwika::rules::turnedBy(
                play.game->world().get<component::Orientation>(play.game->eye(
                        )),
                byYaw,
                byPitch));

        play.game->aimAt(worldRotation(), play.game->cameraTarget());
    }

    void Editor::orbitCamera(
        const float byYaw, const float byPitch)
    {
        const auto backDistance =
            cameraRig.viewHeight
            / std::tan(camera::kEditorFov / 2.0F);
        const auto eye =
            cameraRig.view.transform.position
            - (camera::forward(cameraRig.view.transform) * backDistance);

        cameraRig.view.transform =
            camera::rotated(cameraRig.view.transform, byYaw, byPitch);
        cameraRig.view.transform.position =
            eye + (camera::forward(cameraRig.view.transform) * backDistance);
    }

    void Editor::saveCurrentMap()
    {
        if (document.path().empty())
        {
            dialogs.quitConfirmOpen = false;
            openFileDialog(true);

            return;
        }

        try
        {
            document.map.camera = cameraRig.view;
            const auto hero = playerIndex(document.map);

            if (hero.has_value())
            {
                document.map.characters.at(
                    *hero).idlePlacement = map::Placement{
                    .position = collision::positionOf(
                        play.game->world().get<component::Position>(
                            play.game->player())),
                    .way = play.game->world()
                               .get<component::AnimationState>(
                                   play.game->player())
                               .direction};
            }
            document.map.settings = settingsAsShown();

            for (std::size_t sheet = 0;
                 sheet < map::kAtlasSheetCount;
                 ++sheet)
            {
                image::writePngFile(
                    atlasSheets.sheet(sheet),
                    map::sidecarPath(
                        document.path(),
                        map::kAtlasSheets.at(sheet).name),
                    kAppName);
            }
            saveCharacterSkins();

            if (iconsView.unsaved())
            {
                map::writeSharedTexture(
                    iconsView.sheet(),
                    document.path(),
                    antwika::editor::kIconSheet,
                    kAppName);
                iconsView.keep();
            }

            auto keptMap = document.map;

            keptMap.decor = compactedDecor(document.map.decor);
            map::saveMap(document.path(), keptMap);
            if (const auto notice = plan.save(); notice.has_value())
            {
                showStatus(*notice, true, 600);
                logger.log(log::Level::Warning, *notice);
            }
            document.markSaved();
            showStatus("saved", false, 120);
            logger.log(
                log::Level::Info,
                "Saved " + document.path() + " and both atlases");
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
            document.map = map::loadMap(document.path());
            document.forgetHistory();

            const auto camera = document.map.camera.value_or(
                map::CameraView{
                    .transform = camera::centeredOn(
                        cameraRig.view.transform,
                        voxelmap::voxelsCenter(document.map.voxels)),
                    .zoom = cameraRig.view.zoom});

            cameraRig.view = camera;
            cameraRig.viewHeight =
                camera::orthoHalfHeight(camera::kCanvasSize,
                    cameraRig.view.zoom);
            takeSettings(document.map.settings);
            atlasSheets.take(
                map::loadAtlasPairOrBlank(document.path(), kAppName));
            iconsView.open(
                viewportRenderer, loadIconSheet(document.path(), kAppName));
            characterView.editFirst();
            figurePicked.reset();
            editLevel =
                antwika::voxel::cubeIndexOfLevel(
                    voxelmap::topLevel(document.map.voxels));
            rebuildWorld();
            resetGates();
            standPlayer();
            loadCharacterSkins();
            document.markSaved();
            logger.log(
                log::Level::Info,
                "Loaded " + document.path() + ": "
                    + solver::weaveErrorMessage(
                        voxelmap::visibleFacesOf(document.map.voxels),
                        document.map.rules,
                        solver::solveTiles(
                            voxelmap::visibleFacesOf(document.map.voxels),
                            document.map.rules,
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

        document.map = std::move(emptyMap);
        document.openAt({});
        atlasSheets.take(
            {tilemap::blankAtlas(tilemap::kWallTileSize),
             tilemap::blankAtlas(tilemap::kFloorTileSize)});
        atlasSheets.touch();
        characterView.editFirst();
        figurePicked.reset();
        editLevel =
            antwika::voxel::cubeIndexOfLevel(voxelmap::topLevel(
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

        dialogs.mapEntries = antwika::editor::filterMapNames(names);
    }

    void Editor::openFileDialog(const bool forSave)
    {
        const auto path = std::filesystem::absolute(
            document.path().empty() ? document.startPath() : document.path());
        const auto folder = path.parent_path().string();

        dialogs.fileDialog = FileDialog{
            .isSaveMode = forSave,
            .folder = folder,
            .fileName =
                document.path().empty() ? std::string{} : path.filename(
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
             / antwika::editor::ensureMapExtension(
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

    void Editor::takeSettings(const map::Settings &shownSettings)
    {
        settings = shownSettings;
        activeView = shownSettings.view;
        viewBeforePlay = shownSettings.view;
        cornerJoining = shownSettings.cornersJoined
                      ? solver::CornerSeams::Included
                      : solver::CornerSeams::Ignored;
    }

    map::Settings Editor::settingsAsShown() const
    {
        auto shownSettings = settings;

        shownSettings.view = play.playing ? viewBeforePlay : activeView;
        shownSettings.cornersJoined =
            cornerJoining == solver::CornerSeams::Included;

        return shownSettings;
    } // GCOVR_EXCL_LINE

    map::Snapshot Editor::snapshot()
    {
        return map::Snapshot{
            .map = document.map,
            .pixelBitmaps = atlasSheets.sheets(),
            .characterBitmaps = characterView.skinsAsDrawn()};
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
        growTroublePositions.clear();
        document.map = std::move(stepSnapshot.map);
        atlasSheets.take(std::move(stepSnapshot.pixelBitmaps));
        characterView.takeSkins(
            viewportRenderer, std::move(stepSnapshot.characterBitmaps));
        rebuildWorld();
        spawnRoster();
        atlasSheets.touch();
        characterView.touch();
    }

}
