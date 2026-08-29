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

            if (beginEdgeDrag(interactions))
            {
                return;
            }

            if (interactions.chosenChoice.has_value())
            {
                for (const auto menu : antwika::editor::kEveryMenu)
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
                        dialogs.openMenu.reset();
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

        if (fileChooser.fileDialog.has_value() || dialogs.quitConfirmOpen
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
                        antwika::voxel::getCubeTop(worldView.worldEdit().getEditLevel())));
            }
        }

        if (inkPanel.consumePickerPress(
                downPressed, pointer, getRailWidthOnCanvas()))
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
                    || preferences.tool == Tool::Picker))
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
                    viewChoice.activeView = View::Atlases;
                }

                return;
            }

            const auto cell = voxelmap::getCellUnder(
                getWorldCamera(play, cameraRig),
                getWorldRotation(play),
                camera::kCanvasSize,
                point,
                antwika::voxel::getCubeTop(worldView.worldEdit().getEditLevel()));

            if (!cell.has_value())
            {
                return;
            }

            switch (placementOf(preferences.tool))
            {
            case ToolPlacement::Lamp:
                if (downPressed.button == input::MouseButton::Left
                    && worldView.beginLampCarry(viewContextNow(), *cell))
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
                                  inkPanel.inkPicker.activeInk))
                        : light::withoutLampAt(
                              document.map.lamps, *cell);
                lightPasses.forget();

                return;

            case ToolPlacement::Stamp:
                worldView.pressStamp(
                    viewContextNow(), *cell, downPressed.button);

                return;

            case ToolPlacement::Character:
                pressCharacter(*cell, downPressed.button);

                return;

            case ToolPlacement::Marker:
                pressMarker(*cell, downPressed.button);

                return;

            case ToolPlacement::StartOrExit:
                placeStartOrExit(*cell, downPressed.button);

                return;

            case ToolPlacement::Select:
                pressSelect(*cell, downPressed.button);

                return;

            case ToolPlacement::Shape:
                break;
            }

            if (worldView.beginShape(
                    viewContextNow(), *cell, downPressed.button))
            {
                return;
            }

            pushUndo();

            document.map.voxels = voxel::getWithRampsRebuilt(
                preferences.tool == Tool::Eraser
                      ? voxel::withoutBlockAt(
                          document.map.voxels, *cell)
                    : voxel::withBlockAt(
                          document.map.voxels,
                          *cell,
                          preferences.kind,
                          voxel::Facing::Any),
                *cell);
            worldView.beginPaintDrag(*cell, downPressed.button);
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
            antwika::voxel::getCubeTop(worldView.worldEdit().getEditLevel()));

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
            worldView.pressStamp(
                viewContextNow(), *cell, input::MouseButton::Right);

            return;

        case ToolPlacement::Character:
            pressCharacter(*cell, input::MouseButton::Right);

            return;

        case ToolPlacement::Marker:
            pressMarker(*cell, input::MouseButton::Right);

            return;

        case ToolPlacement::StartOrExit:
            placeStartOrExit(*cell, input::MouseButton::Right);

            return;

        case ToolPlacement::Select:
            pressSelect(*cell, input::MouseButton::Right);

            return;

        case ToolPlacement::Shape:
            break;
        }
    }

}
