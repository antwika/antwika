#include <antwika/gfx/PointF.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/light/PointLight.hpp>
#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/voxelmap/VoxelPick.hpp>

#include "antwika/editor/Editor.hpp"

namespace antwika::editor
{

    void Editor::onPointerPressed(
        const input::PointerButtonPressed &downPressed)
    {
        if (play.playing)
        {
            if (play.titleScreenUp)
            {
                play.titleScreenUp = false;

                return;
            }

            if (downPressed.button == input::MouseButton::Middle)
            {
                turningPlayer = true;
                pointer.lastPointerPosition = downPressed.position;
            }

            if (downPressed.button == input::MouseButton::Left)
            {
                pathTo(downPressed.position);
            }

            return;
        }

        if (downPressed.button == input::MouseButton::Left)
        {
            pointer.pointerHeld = true;
            pointer.pointerInWindow = antwika::gfx::Point{
                .x = downPressed.position.x,
                .y = downPressed.position.y};

            const auto frame = layoutUi(true, true);
            const auto &interactions = frame.interactions;

            if (beginSliderDrag(interactions))
            {
                return;
            }

            if (interactions.chosenChoice.has_value())
            {
                for (const auto menu :
                     {antwika::editor::Menu::File,
                 antwika::editor::Menu::Edit,
                      antwika::editor::Menu::View,
                      antwika::editor::Menu::Settings})
                {
                    if (interactions.chosenChoice->dropdownWidget
                        != getMenuWidget(menu))
                    {
                        continue;
                    }

                    const auto item =
                        antwika::editor::itemAt(
                            menu,
                            interactions.chosenChoice->index);

                    if (item.has_value())
                    {
                        dialogs.openMenu =
                            *item
                                    == antwika::editor::
                                        MenuItem::
                                            Settings
                                ? std::optional{
                                      antwika::editor::
                                          Menu::
                                              Settings}
                                : std::nullopt;
                        onMenuItem(*item);
                    }
                }

                return;
            }

            if (interactions.activatedWidget
                    != antwika::widget::kNoWidget
                && consumeWidgets(interactions))
            {
                return;
            }

            if (interactions.pointerOverUi)
            {
                dialogs.openMenu.reset();

                return;
            }

            dialogs.openMenu.reset();
        }

        if (dialogs.fileDialog.has_value() || dialogs.quitConfirmOpen
            || keyBench.panelShown)
        {
            return;
        }

        if (downPressed.button == input::MouseButton::Middle)
        {
            cameraRig.panning = true;
            pointer.lastPointerPosition = downPressed.position;
            cameraRig.panGripPosition.reset();

            if (isWorldShown() && !play.playing)
            {
                const auto projectToScreen =
                    viewportRenderer.getViewport().toCanvas(
                        antwika::gfx::Point{
                            .x = downPressed.position.x,
                            .y = downPressed.position.y});

                cameraRig.panGripPosition = voxelmap::getPlaneHit(
                    voxelmap::getRayInModelSpace(
                        voxelmap::getRayThrough(
                            getWorldCamera(play, cameraRig),
                            camera::kCanvasSize,
                            antwika::gfx::PointF{
                                static_cast<float>(
                                    projectToScreen.x),
                                static_cast<float>(
                                    projectToScreen.y)}),
                        getWorldRotation(play)),
                    static_cast<float>(
                        antwika::voxel::getCubeTop(worldView.worldEdit.editLevel)));
            }
        }

        if (consumePickerPress(downPressed))
        {
            return;
        }

        if (isWorldShown()
            && downPressed.button == input::MouseButton::Right)
        {
            cameraRig.orbitFromPosition = downPressed.position;
            cameraRig.orbiting = false;

            return;
        }

        if (isWorldShown()
            && downPressed.button == input::MouseButton::Left)
        {
            const auto projectToScreen =
                viewportRenderer.getViewport().toCanvas(
                    antwika::gfx::Point{
                        .x = downPressed.position.x,
                        .y = downPressed.position.y});
            const antwika::gfx::PointF point{
                static_cast<float>(projectToScreen.x),
                static_cast<float>(projectToScreen.y)};

            if (downPressed.button == input::MouseButton::Left
                && (getHeldModifiers().shift
                    || preferences.tool == map::Tool::Picker))
            {
                const auto pickedFace = voxelmap::getTilePicked(
                    visibleCells(),
                    worldMeshes.getFaces(),
                    worldMeshes.getDrawnAs(),
                    getWorldCamera(play, cameraRig),
                    getWorldRotation(play),
                    camera::kCanvasSize,
                    point);

                if (pickedFace.has_value())
                {
                    stroke.selectedTile = pickedFace;
                    stroke.selectedEdges.reset();
                    stroke.dragFromCell.reset();
                    stroke.dragFromPoint.reset();
                    viewChoice.activeView = map::View::Atlases;
                }

                return;
            }

            const auto cell = voxelmap::getCellUnder(
                getWorldCamera(play, cameraRig),
                getWorldRotation(play),
                camera::kCanvasSize,
                point,
                antwika::voxel::getCubeTop(worldView.worldEdit.editLevel));

            if (!cell.has_value())
            {
                return;
            }

            switch (placementOf(preferences.tool))
            {
            case ToolPlacement::Lamp:
                if (downPressed.button == input::MouseButton::Left
                    && beginLampCarry(*cell))
                {
                    return;
                }

                pushUndo();

                document.map.lamps =
                    downPressed.button == input::MouseButton::Left
                                        ? light::withLampAt(
                              document.map.lamps,
                              *cell,
                              document.map.paletteColors.at(
                                  inkPicker.activeInk))
                        : light::withoutLampAt(
                              document.map.lamps, *cell);
                lightPasses.forget();

                return;

            case ToolPlacement::Stamp:
                pressStamp(*cell, downPressed.button);

                return;

            case ToolPlacement::Figure:
                pressFigure(*cell, downPressed.button);

                return;

            case ToolPlacement::Plate:
                pressPlate(*cell, downPressed.button);

                return;

            case ToolPlacement::Gate:
                pressGate(*cell, downPressed.button);

                return;

            case ToolPlacement::StartOrExit:
                placeStartOrExit(*cell, downPressed.button);

                return;

            case ToolPlacement::Shape:
                break;
            }

            if (beginShape(*cell, downPressed.button))
            {
                return;
            }

            pushUndo();

            document.map.voxels = voxel::getWithRampsRebuilt(
                preferences.tool == map::Tool::Eraser
                      ? voxel::withoutBlockAt(
                          document.map.voxels, *cell)
                    : voxel::withBlockAt(
                          document.map.voxels,
                          *cell,
                          preferences.kind,
                          rampFacing),
                *cell);
            worldView.worldPaint.dragButton = downPressed.button;
            worldView.worldPaint.lastPaintedPosition = cell;
            rebuildWorld();

            return;
        }

        pressedOnSheets(downPressed);
    }

    void Editor::rightTaken(const input::Position position)
    {
        const auto projectToScreen = viewportRenderer.getViewport().toCanvas(
            antwika::gfx::Point{.x = position.x, .y = position.y});
        const auto cell = voxelmap::getCellUnder(
            getWorldCamera(play, cameraRig),
            getWorldRotation(play),
            camera::kCanvasSize,
            antwika::gfx::PointF{
                static_cast<float>(projectToScreen.x),
                static_cast<float>(projectToScreen.y)},
            antwika::voxel::getCubeTop(worldView.worldEdit.editLevel));

        if (!cell.has_value())
        {
            return;
        }

        switch (placementOf(preferences.tool))
        {
        case ToolPlacement::Lamp:
            pushUndo();
            document.map.lamps = light::withoutLampAt(document.map.lamps,
                *cell);
            lightPasses.forget();

            return;

        case ToolPlacement::Stamp:
            pressStamp(*cell, input::MouseButton::Right);

            return;

        case ToolPlacement::Figure:
            pressFigure(*cell, input::MouseButton::Right);

            return;

        case ToolPlacement::Plate:
            pressPlate(*cell, input::MouseButton::Right);

            return;

        case ToolPlacement::Gate:
            pressGate(*cell, input::MouseButton::Right);

            return;

        case ToolPlacement::StartOrExit:
            placeStartOrExit(*cell, input::MouseButton::Right);

            return;

        case ToolPlacement::Shape:
            break;
        }
    }

}
