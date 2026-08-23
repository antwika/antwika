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
        game->stepAlongPath(playing);
        moveCamera();
        game->setWalkerFrozen(
            !playing || heldModifiers().control || heldModifiers().shift
                || heldModifiers().alt);
        game->setWorldFrozen(!playing);
        game->setSpeaking(
            tick < caption.untilTick && caption.speaker.has_value()
                ? std::optional<std::uint32_t>(
                      static_cast<std::uint32_t>(*caption.speaker))
                : std::nullopt);
        game->setRosterCount(document.map.characters.size());
        game->run(tick);
        sayConsumeReport();
        sayDialogueLine();

        if (playing && !game->world().alive(game->player()))
        {
            sayCaption("the walker", "it gave out");
            standPlayer();
        }

        const auto walkerStood =
            game->world().get<component::Position>(game->player());
        const antwika::gfx::Vec3 walkerPosition{
            walkerStood.x, walkerStood.y, walkerStood.z};

        if (playing)
        {
            onSteppedWorld(walkerPosition);
        }

        if (cameraFollows && playing)
        {
            game->follow(worldRotation(), walkerPosition);
        }

        ++tick;
    }

#ifdef ANTWIKA_GAME_SHARED
    void Editor::reloadGameModule()
    {
        if (!game.hasChanged())
        {
            return;
        }

        const auto keptTransform = game->cameraTransform();
        const auto keptZoom = game->zoom();
        const auto target = game->cameraTarget();
        const auto gates = game->gates();

        if (!game.reload())
        {
            return;
        }

        for (const auto standing : world.view<component::Player>())
        {
            game->setPlayer(standing);

            break;
        }

        game->cameraTransform() = keptTransform;
        game->zoom() = keptZoom;
        game->cameraTarget() = target;
        game->gates() = gates;

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

        if (remeshPending)
        {
            remeshPending = false;
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

        const auto model = worldRotation();
        const auto walkerStood =
            game->world().get<component::Position>(game->player());
        const antwika::gfx::Vec3 walkerPosition{
            walkerStood.x, walkerStood.y, walkerStood.z};

        if (activeView == map::View::World)
        {
            const auto aimedRotation =
                playing
                    ? std::optional<
                          antwika::voxel::VoxelPosition>{}
                    : voxelmap::cellUnder(
                          worldCamera(),
                          model,
                          camera::kCanvasSize,
                          pointer.pointerOnCanvas,
                          antwika::voxel::cubeTop(editLevel));
            const auto anchoredTarget =
                playing
                    ? walkerPosition
                    : antwika::gfx::Vec3{
                          static_cast<float>(
                              aimedRotation.value_or(
                                        antwika::voxel::
                                            VoxelPosition{})
                                  .x)
                              + 0.5F,
                          static_cast<float>(
                              antwika::voxel::cubeTop(
                                  editLevel))
                              + 0.5F,
                          static_cast<float>(
                              aimedRotation.value_or(
                                        antwika::voxel::
                                            VoxelPosition{})
                                  .z)
                              + 0.5F};
            const auto hideFrom = clockSource.now();
            auto behind =
                playing || aimedRotation.has_value()
                    ? antwika::voxel::occludingVoxels(
                          worldMeshes.cells(), anchoredTarget)
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
                    clockSource.now() - hideFrom));

            const auto corner =
                voxelmap::occlusionMaskOrigin(aboutPosition);
            const auto lights = currentLights();

            const auto lampFrom = clockSource.now();

            lightPasses.bakeLamps(
                viewportRenderer,
                worldMeshes.solid(),
                lights);

            meters.lampRate.record(
                std::chrono::duration_cast<
                    std::chrono::nanoseconds>(
                    clockSource.now() - lampFrom));

            worldShader.setLook(
                viewportRenderer,
                render::WorldShaderInputs{
                    .playing = playing,
                    .lighting = lighting,
                    .sightOn = lampSight && playing && lowerLight,
                    .ambient =
                        static_cast<float>(document.map.ambient) / 100.0F,
                    .walkerPosition = walkerPosition,
                    .fadeAbove =
                        playing
                            ? walkerStood.y
                            : (static_cast<float>(
                                   antwika::voxel::cubeTop(editLevel)
                                   - voxel::kCubeSide)
                               + 0.5F)
                                  * antwika::voxel::kVoxelSide,
                    .carrying = std::optional<std::size_t>{},
                    .hidingCornerPosition = corner,
                    .sightPoint =
                        antwika::voxel::lineOfSight(walkerPosition),
                    .sightSlot = 0,
                    .upperSightPoint =
                        antwika::voxel::upperLineOfSight(walkerPosition),
                    .upperSightSlot = 1},
                lights,
                lightPasses.lamps());
        }

        atlasSheets.refresh(
            viewportRenderer, document.map, tick, shouldAdvanceTileAnimation());

        characterView.refresh(viewportRenderer);

        viewportRenderer.clear(
            playing ? kPlayBackgroundColor : kEditorBackgroundColor);

        if (activeView == map::View::Atlases && selectedTile.has_value())
        {
            const auto seedNow =
                previewAuto ? tick / 62 : previewSeed;

            if (previewForTile != selectedTile
                || previewLayer != chosenLayer
                || seedNow != previewSeed)
            {
                previewForTile = selectedTile;
                previewLayer = chosenLayer;
                previewSeed = seedNow;
                previewTiles = decor::previewNeighbourhood(
                    activeRules(), *selectedTile, 3, previewSeed);
            }
        }

        const auto uiResting =
            playing && !dialogs.quitConfirmOpen && !keysOpen
            && !dialogs.fileDialog.has_value()
            && !inkPicker.editingInk.has_value()
            && !rebindingAction.has_value() && !slidingWidget.has_value()
            && focusedField == FocusedField::Nothing;

        const auto uiFrom = clockSource.now();
        const auto uiFrame = uiResting
                           ? ui::Frame{}
                           : layoutUi(false, pointer.pointerHeld);

        meters.uiRate.record(
            std::chrono::duration_cast<
                std::chrono::nanoseconds>(
                clockSource.now() - uiFrom));

        pointer.hoveredWidget = uiFrame.interactions.hoveredWidget;
        pointer.hoverTracker = updateHover(
            pointer.hoverTracker, pointer.hoveredWidget, tick);
        updateCanvasHover(uiFrame);
        plan.updateFrame(uiFrame, pointer.pointerInWindow);

        if (activeView == map::View::Plan)
        {
            plan.carryEdits(uiFrame, focusedField);
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

            const auto parsedColor = colorFromHex(inkPicker.hexText);

            if (parsedColor.has_value())
            {
                recolorInk(*parsedColor);
                inkPicker.pickerHsv = hsvOf(*parsedColor);
            }
        }

        if (slidingWidget == decor::kFrequencyWidget
            && selectedTile.has_value()
            && uiFrame.interactions.slidChange.has_value()
            && uiFrame.interactions.slidChange->sliderWidget
                   == decor::kFrequencyWidget)
        {
            document.map.decor = withFrequencySet(
                document.map.decor,
                *selectedTile,
                static_cast<std::uint8_t>(
                    uiFrame.interactions.slidChange->value));
        }

        if (slidingWidget == decor::kDecorWeightWidget
            && selectedTile.has_value()
            && uiFrame.interactions.slidChange.has_value()
            && uiFrame.interactions.slidChange->sliderWidget
                   == decor::kDecorWeightWidget)
        {
            document.map.decor = withWeightSet(
                document.map.decor,
                *selectedTile,
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
            && selectedTile.has_value()
            && uiFrame.interactions.slidChange.has_value()
            && uiFrame.interactions.slidChange->sliderWidget
                   == decor::kVariantWeightWidget)
        {
            document.map.familyGroups = withVariantWeightSet(
                document.map.familyGroups,
                *selectedTile,
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

        if (focusedField == FocusedField::FigureName && figurePicked.has_value()
            && *figurePicked < document.map.characters.size()
            && uiFrame.interactions.edit.has_value()
            && uiFrame.interactions.edit->fieldWidget
                   == antwika::editor::kFigureNameWidget)
        {
            document.map.characters.at(*figurePicked).name =
                uiFrame.interactions.edit->text;
        }

        if (focusedField == FocusedField::FigureLine
            && uiFrame.interactions.edit.has_value()
            && uiFrame.interactions.edit->fieldWidget
                   == antwika::editor::kFigureLineWidget)
        {
            pendingFigureLine = uiFrame.interactions.edit->text;
        }

        if (remeshAfterNudge && tick >= lastWheelNudgeTick + 15)
        {
            remeshAfterNudge = false;
            rebuildWorld();
        }

        typedThisFrame.clear();
        keysNow.clear();

        if (activeView == map::View::Character)
        {
            drawCharacterView(uiFrame, startedAt);

            return;
        }

        if (activeView == map::View::Icons)
        {
            drawIconsView(uiFrame, startedAt);

            return;
        }

        if (activeView == map::View::Atlases)
        {
            drawAtlasesView(uiFrame, startedAt);

            return;
        }

        if (activeView == map::View::Plan)
        {
            drawPlanView(uiFrame, startedAt);

            return;
        }

        drawWorldView(uiFrame, startedAt);
    }

}
