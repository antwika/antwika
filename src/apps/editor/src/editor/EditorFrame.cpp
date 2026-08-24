#include <antwika/component/Player.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/decor/Decor.hpp>
#include <antwika/editor/ui/EditorLook.hpp>
#include <antwika/editor/ui/MapPicker.hpp>
#include <antwika/gfx/Camera3D.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/gfx/MeshMaterial.hpp>
#include <antwika/light/PointLight.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/ui/HoverHint.hpp>
#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/voxel/VoxelOcclusion.hpp>
#include <antwika/voxelmap/VoxelPick.hpp>

#include "antwika/editor/Editor.hpp"

namespace antwika::editor
{

    void Editor::simulate()
    {
        play.game->stepAlongPath(play.playing);
        moveCamera();
        play.game->setWalkerFrozen(
            !play.playing || getHeldModifiers().control || getHeldModifiers().shift
                || getHeldModifiers().alt);
        play.game->setWorldFrozen(!play.playing);
        play.game->setSpeaking(
            tick < caption.untilTick && caption.speaker.has_value()
                ? std::optional<std::uint32_t>(
                      static_cast<std::uint32_t>(*caption.speaker))
                : std::nullopt);
        play.game->setRosterCount(document.map.characters.size());
        play.game->run(tick);
        sayConsumeReport();
        sayDialogueLine();

        if (play.playing && !play.game->getWorld().isAlive(play.game->getPlayer()))
        {
            sayCaption("the walker", "it gave out");
            standPlayer();
        }

        const auto walkerStood =
            play.game->getWorld().get<component::Position>(play.game->getPlayer());
        const antwika::gfx::Vec3 walkerPosition{
            walkerStood.x, walkerStood.y, walkerStood.z};

        if (play.playing)
        {
            onSteppedWorld(walkerPosition);
        }

        if (preferences.cameraFollows && play.playing)
        {
            play.game->follow(getWorldRotation(play), walkerPosition);
        }

        ++tick;
    }

#ifdef ANTWIKA_GAME_SHARED
    void Editor::reloadGameModule()
    {
        if (!play.game.hasChanged())
        {
            return;
        }

        const auto keptTransform = play.game->getCameraTransform();
        const auto keptZoom = play.game->zoom();
        const auto target = play.game->cameraTarget();
        const auto gates = play.game->getGates();

        if (!play.game.reload())
        {
            return;
        }

        for (const auto standing : play.world.view<component::Player>())
        {
            play.game->setPlayer(standing);

            break;
        }

        play.game->getCameraTransform() = keptTransform;
        play.game->zoom() = keptZoom;
        play.game->cameraTarget() = target;
        play.game->getGates() = gates;

        logger.log(log::Level::Info, "the game module was reloaded");
        showStatus("the game module was reloaded");
    }
#endif

    void Editor::frame(
        const std::chrono::time_point<std::chrono::system_clock> startedAt)
    {
#ifdef ANTWIKA_GAME_SHARED
        reloadGameModule();
#endif

        if (remesh.pending)
        {
            remesh.pending = false;
            rebuildWorld();
        }

        for (std::size_t tickCount = 0;
             tickDebt.owesTick() && tickCount < app::kMaxCatchUpTicks;
             ++tickCount)
        {
            tickDebt.payTick();
            simulate();
        }

        if (tickDebt.owesTick())
        {
            tickDebt.forgive();
        }

        const auto model = getWorldRotation(play);
        const auto walkerStood =
            play.game->getWorld().get<component::Position>(play.game->getPlayer());
        const antwika::gfx::Vec3 walkerPosition{
            walkerStood.x, walkerStood.y, walkerStood.z};

        if (isWorldShown())
        {
            const auto aimedRotation =
                play.playing
                    ? std::optional<
                          antwika::voxel::VoxelPosition>{}
                    : voxelmap::getCellUnder(
                          getWorldCamera(play, cameraRig),
                          model,
                          camera::kCanvasSize,
                          pointer.pointerOnCanvas,
                          antwika::voxel::getCubeTop(worldView.worldEdit.editLevel));
            const auto anchoredTarget =
                play.playing
                    ? walkerPosition
                    : antwika::gfx::Vec3{
                          static_cast<float>(
                              aimedRotation.value_or(
                                        antwika::voxel::
                                            VoxelPosition{})
                                  .x)
                              + 0.5F,
                          static_cast<float>(
                              antwika::voxel::getCubeTop(
                                  worldView.worldEdit.editLevel))
                              + 0.5F,
                          static_cast<float>(
                              aimedRotation.value_or(
                                        antwika::voxel::
                                            VoxelPosition{})
                                  .z)
                              + 0.5F};
            const auto hideFrom = clockSource.getCurrentTime();
            auto behind =
                play.playing || aimedRotation.has_value()
                    ? antwika::voxel::getOccludingVoxels(
                          worldMeshes.getCells(), anchoredTarget)
                    : antwika::voxel::Voxels{};

            pointer.hoveredPosition = aimedRotation;

            if (aimedRotation.has_value())
            {
                std::erase_if(
                    behind,
                    [pad = antwika::voxel::cubeCornerOf(*aimedRotation)](
                        const auto &standing)
                    {
                        return antwika::voxel::cubeCornerOf(standing.first)
                               == pad;
                    });
            }

            const antwika::voxel::VoxelPosition aboutPosition{
                .x = static_cast<std::int32_t>(
                    std::floor(
                        anchoredTarget.x / antwika::voxel::kVoxelSide)),
                .z = static_cast<std::int32_t>(
                    std::floor(
                        anchoredTarget.z / antwika::voxel::kVoxelSide))};

            lightPasses.hide(viewportRenderer, std::move(behind),
                aboutPosition);

            meters.hideRate.record(
                std::chrono::duration_cast<
                    std::chrono::nanoseconds>(
                    clockSource.getCurrentTime() - hideFrom));

            const auto corner =
                voxelmap::getOcclusionMaskOrigin(aboutPosition);
            const auto lights = currentLights();

            const auto lampFrom = clockSource.getCurrentTime();

            lightPasses.bakeLamps(
                viewportRenderer,
                worldMeshes.getSolid(),
                lights);

            meters.lampRate.record(
                std::chrono::duration_cast<
                    std::chrono::nanoseconds>(
                    clockSource.getCurrentTime() - lampFrom));

            worldShader.setLook(
                viewportRenderer,
                render::WorldShaderInputs{
                    .playing = play.playing,
                    .lighting = document.map.settings.lighting,
                    .sightOn = preferences.lampSight && play.playing
                               && worldView.worldEdit.lowerLight,
                    .ambient =
                        static_cast<float>(document.map.ambient) / 100.0F,
                    .walkerPosition = walkerPosition,
                    .fadeAbove =
                        play.playing
                            ? walkerStood.y
                            : (static_cast<float>(
                                   antwika::voxel::getCubeTop(worldView.worldEdit.editLevel)
                                   - voxel::kCubeSide)
                               + 0.5F)
                                  * antwika::voxel::kVoxelSide,
                    .carrying = std::optional<std::size_t>{},
                    .hidingCornerPosition = corner,
                    .sightPoint =
                        antwika::voxel::getLineOfSight(walkerPosition),
                    .sightSlot = 0,
                    .upperSightPoint =
                        antwika::voxel::getUpperLineOfSight(walkerPosition),
                    .upperSightSlot = 1,
                    .upperSightOn = worldView.isUpperSightOn(viewContextNow())},
                lights,
                lightPasses.getLamps());
        }

        atlasSheets.refresh(
            viewportRenderer, document.map, tick, shouldAdvanceTileAnimation());

        characterView.refresh(viewportRenderer);

        viewportRenderer.clear(
            play.playing ? kPlayBackgroundColor : kEditorBackgroundColor);

        if (viewChoice.activeView == map::View::Atlases && stroke.selectedTile.has_value())
        {
            const auto seedNow =
                preview.automatic ? tick / 62 : preview.seed;

            if (preview.forTile != stroke.selectedTile
                || preview.layer != chosenLayer
                || seedNow != preview.seed)
            {
                preview.forTile = stroke.selectedTile;
                preview.layer = chosenLayer;
                preview.seed = seedNow;
                preview.tiles = decor::getPreviewNeighbourhood(
                    getActiveRules(
                        document.map, chosenLayer), *stroke.selectedTile, 3, preview.seed);
            }
        }

        const auto uiResting =
            play.playing && !dialogs.quitConfirmOpen && !keyBench.panelShown
            && !dialogs.fileDialog.has_value()
            && !inkPicker.editingInk.has_value()
            && !keyBench.rebindingAction.has_value() && !slidingWidget.has_value()
            && focusedField == FocusedField::Nothing;

        const auto uiFrom = clockSource.getCurrentTime();
        const auto uiFrame = uiResting
                           ? ui::Frame{}
                           : layoutUi(false, pointer.pointerHeld);

        meters.uiRate.record(
            std::chrono::duration_cast<
                std::chrono::nanoseconds>(
                clockSource.getCurrentTime() - uiFrom));

        pointer.hoveredWidget = uiFrame.interactions.hoveredWidget;
        pointer.hoverTracker = getUpdateHover(
            pointer.hoverTracker, pointer.hoveredWidget, tick);
        updateCanvasHover(uiFrame);
        plan.updateFrame(uiFrame, pointer.pointerInWindow);

        if (auto *view = viewNow(); view != nullptr)
        {
            view->carryFrame(uiFrame, viewContextNow());
        }

        if (dialogs.fileDialog.has_value() && dialogs.fileDialog->isSaveMode
            && uiFrame.interactions.edit.has_value()
            && uiFrame.interactions.edit->fieldWidget
                   == antwika::editor::kPickerNameWidget)
        {
            dialogs.fileDialog->fileName =
                uiFrame.interactions.edit->text;
        }

        if (inkPicker.editingInk.has_value()
            && uiFrame.interactions.edit.has_value()
            && uiFrame.interactions.edit->fieldWidget
                   == decor::kInkHexWidget)
        {
            inkPicker.hexText = uiFrame.interactions.edit->text;

            const auto parsedColor = getColorFromHex(inkPicker.hexText);

            if (parsedColor.has_value())
            {
                recolorInk(*parsedColor);
                inkPicker.pickerHsv = hsvOf(*parsedColor);
            }
        }

        if (slidingWidget == decor::kFrequencyWidget
            && stroke.selectedTile.has_value()
            && uiFrame.interactions.slidChange.has_value()
            && uiFrame.interactions.slidChange->sliderWidget
                   == decor::kFrequencyWidget)
        {
            document.map.decor = getWithFrequencySet(
                document.map.decor,
                *stroke.selectedTile,
                static_cast<std::uint8_t>(
                    uiFrame.interactions.slidChange->value));
        }

        if (slidingWidget == decor::kDecorWeightWidget
            && stroke.selectedTile.has_value()
            && uiFrame.interactions.slidChange.has_value()
            && uiFrame.interactions.slidChange->sliderWidget
                   == decor::kDecorWeightWidget)
        {
            document.map.decor = getWithWeightSet(
                document.map.decor,
                *stroke.selectedTile,
                static_cast<std::uint8_t>(
                    uiFrame.interactions.slidChange->value));
        }

        if (slidingWidget == antwika::editor::kGlowWidget
            && inkPicker.editingInk.has_value()
            && *inkPicker.editingInk < document.map.glows.size()
            && uiFrame.interactions.slidChange.has_value()
            && uiFrame.interactions.slidChange->sliderWidget
                   == antwika::editor::kGlowWidget)
        {
            document.map.glows.at(*inkPicker.editingInk) =
                static_cast<std::uint8_t>(
                    uiFrame.interactions.slidChange->value);
        }

        if (slidingWidget == antwika::editor::kAmbientWidget
            && uiFrame.interactions.slidChange.has_value()
            && uiFrame.interactions.slidChange->sliderWidget
                   == antwika::editor::kAmbientWidget)
        {
            document.map.ambient = static_cast<std::uint8_t>(
                uiFrame.interactions.slidChange->value);
        }

        if (slidingWidget
                == decor::kVariantWeightWidget
            && stroke.selectedTile.has_value()
            && uiFrame.interactions.slidChange.has_value()
            && uiFrame.interactions.slidChange->sliderWidget
                   == decor::kVariantWeightWidget)
        {
            document.map.familyGroups = getWithVariantWeightSet(
                document.map.familyGroups,
                *stroke.selectedTile,
                static_cast<std::uint8_t>(
                    uiFrame.interactions.slidChange->value));
        }

        if (focusedField == FocusedField::ExitTarget
            && uiFrame.interactions.edit.has_value()
            && uiFrame.interactions.edit->fieldWidget
                   == antwika::editor::kExitTargetWidget)
        {
            document.map.exitTarget = uiFrame.interactions.edit->text;
        }

        if (focusedField == FocusedField::FigureName && worldView.figureTool.chosenIndex.has_value()
            && *worldView.figureTool.chosenIndex < document.map.characters.size()
            && uiFrame.interactions.edit.has_value()
            && uiFrame.interactions.edit->fieldWidget
                   == antwika::editor::kFigureNameWidget)
        {
            document.map.characters.at(*worldView.figureTool.chosenIndex).name =
                uiFrame.interactions.edit->text;
        }

        if (focusedField == FocusedField::FigureLine
            && uiFrame.interactions.edit.has_value()
            && uiFrame.interactions.edit->fieldWidget
                   == antwika::editor::kFigureLineWidget)
        {
            worldView.figureTool.pendingLine = uiFrame.interactions.edit->text;
        }

        if (remesh.afterNudge && tick >= remesh.lastWheelNudgeTick + 15)
        {
            remesh.afterNudge = false;
            rebuildWorld();
        }

        keyBench.typedThisFrame.clear();
        keyBench.keysNow.clear();

        if (auto *view = viewNow(); view != nullptr)
        {
            view->draw(viewContextNow(), uiFrame);
            finishView(uiFrame, startedAt);
        }
    }

}
