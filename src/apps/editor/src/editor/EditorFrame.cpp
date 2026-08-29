#include <antwika/component/Player.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/decor/Decor.hpp>
#include <antwika/editor/ui/EditorLook.hpp>
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
#include "antwika/editor/ui/WidgetCatalog.hpp"

namespace antwika::editor
{

    void Editor::simulate()
    {
        play.game->stepAlongPath(play.playing);
        moveCamera();
        play.simulationState.walkerHeld =
            !play.playing || getHeldModifiers().control
            || getHeldModifiers().shift || getHeldModifiers().alt;
        play.simulationState.simulationPaused = !play.playing;
        play.simulationState.speaking =
            tick < simulation.caption.untilTick
                    && simulation.caption.speaker.has_value()
                ? std::optional<std::uint32_t>(
                      static_cast<std::uint32_t>(*simulation.caption.speaker))
                : std::nullopt;
        play.simulationState.characterCount = document.map.characters.size();
        play.game->setSimulation(play.simulationState);
        play.game->run(tick);
        simulation.sayConsumeReport(*play.game);
        simulation.sayDialogueLine(*play.game);

        if (play.playing && !play.game->getWorld().isAlive(play.game->getPlayer()))
        {
            simulation.sayCaption("the walker", "it gave out");
            standPlayer();
        }

        const auto walkerStood =
            play.game->getWorld().get<component::Position>(play.game->getPlayer());
        const antwika::gfx::Vec3 walkerPosition{
            walkerStood.x, walkerStood.y, walkerStood.z};

        if (play.playing)
        {
            onSteppedWorld();
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
        const auto keptZoom = play.game->getZoom();
        const auto target = play.game->getCameraTarget();
        const auto checkpoint = play.game->getCheckpoint();

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
        play.game->setZoom(keptZoom);
        play.game->setCameraTarget(target);
        play.game->setCheckpoint(checkpoint);

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
             running && tickDebt.owesTick()
             && tickCount < app::kMaxCatchUpTicks;
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
                          antwika::voxel::getCubeTop(worldView.worldEdit().getEditLevel()));
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
                                  worldView.worldEdit().getEditLevel()))
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
                    .lighting = play.playing
                                    ? document.map.settings.lighting
                                    : preferences.lighting,
                    .sightOn = preferences.lampSight && play.playing
                               && worldView.worldEdit().lowerLight,
                    .ambient =
                        static_cast<float>(document.map.ambient) / 100.0F,
                    .walkerPosition = walkerPosition,
                    .fadeAbove =
                        play.playing
                            ? walkerStood.y
                            : (static_cast<float>(
                                   antwika::voxel::getCubeTop(
                                       worldView.worldEdit().getEditLevel())
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
                    .upperSightOn = worldView.isUpperSightOn(viewContextNow()),
                    .viewPosition =
                        getWorldCamera(play, cameraRig).getPosition(),
                    .viewTargetPoint =
                        getWorldCamera(play, cameraRig).getTarget(),
                    .backdropColor = kPlayBackgroundColor},
                lights,
                lightPasses.getLamps());
        }

        atlasSheets.refresh(
            viewportRenderer, document.map, tick, shouldAdvanceTileAnimation());

        characterView.refresh(viewportRenderer);

        viewportRenderer.clear(
            play.playing ? kPlayBackgroundColor : kEditorBackgroundColor);

        if (viewChoice.activeView == View::Atlases && stroke.selectedTile.has_value())
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
            && !fileChooser.fileDialog.has_value()
            && !inkPanel.inkPicker.editingInk.has_value()
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

        if (uiFrame.interactions.edit.has_value())
        {
            for (const auto &row : getWidgetCatalog().fieldRows)
            {
                if (uiFrame.interactions.edit->fieldWidget == row.widget)
                {
                    row.editEffect(*this, uiFrame.interactions.edit->text);
                }
            }

            widget_catalog::carryFamilyEdit(
                getWidgetCatalog(),
                *this,
                uiFrame.interactions.edit->fieldWidget,
                uiFrame.interactions.edit->text);
        }

        if (uiFrame.interactions.slidChange.has_value())
        {
            for (const auto &row : getWidgetCatalog().sliderRows)
            {
                if (slidingWidget == row.widget
                    && uiFrame.interactions.slidChange->sliderWidget
                           == row.widget)
                {
                    row.slideEffect(
                        *this, uiFrame.interactions.slidChange->value);
                }
            }
        }

        if (uiFrame.interactions.edge.has_value()
            && pointer.heldEdgeWidget
                   == uiFrame.interactions.edge->edgeWidget)
        {
            static_cast<void>(beginEdgeDrag(uiFrame.interactions));
        }

        carryComponentScroll(uiFrame.interactions);
        carryEntityListScroll(uiFrame.interactions);

        if (remesh.afterNudge && tick >= remesh.lastWheelNudgeTick + 15)
        {
            remesh.afterNudge = false;
            rebuildWorld();
        }

        keyBench.typedThisFrame.clear();
        keyBench.keysNow.clear();
        pointer.wheelSteps = 0;

        if (auto *view = viewNow(); view != nullptr)
        {
            view->draw(viewContextNow(), uiFrame);
            finishView(uiFrame, startedAt);
        }
    }

}
