#include <antwika/editor/ui/AtlasView.hpp>
#include <antwika/editor/ui/CharacterView.hpp>
#include <antwika/editor/ui/EditorLook.hpp>
#include <antwika/editor/ui/IconSheet.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/gfx/PointF.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/tilemap/Tilemap.hpp>
#include <antwika/tile/TilePaint.hpp>
#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/voxelmap/VoxelPick.hpp>

#include "antwika/editor/Editor.hpp"

namespace antwika::editor
{

    namespace
    {
    }

    void Editor::onPointerMoved(const input::PointerMoved &movedEvent)
    {
        const auto projectToScreen =
            viewportRenderer.getViewport().toCanvas(
                antwika::gfx::Point{
                    .x = movedEvent.position.x,
                    .y = movedEvent.position.y});

        pointer.pointerOnCanvas = antwika::gfx::PointF{
            static_cast<float>(projectToScreen.x),
            static_cast<float>(projectToScreen.y)};
        pointer.pointerInWindow = antwika::gfx::Point{
            .x = movedEvent.position.x,
            .y = movedEvent.position.y};

        plan.draggedTo(
            antwika::gfx::Point{
                .x = movedEvent.position.x, .y = movedEvent.position.y});

        if (play.playing)
        {
            if (turningPlayer)
            {
                turnPlayer(
                    static_cast<float>(
                        pointer.lastPointerPosition.x - movedEvent.position.x)
                        * camera::kMouseTurn,
                    static_cast<float>(
                        movedEvent.position.y - pointer.lastPointerPosition.y)
                        * camera::kMouseTurn);
            }

            pointer.lastPointerPosition = movedEvent.position;
            return;
        }

        if (inkPicker.pickerDragging && inkPicker.editingInk.has_value())
        {
            const auto takenColor =
                getColorAtPoint(camera::kCanvasSize, inkPicker.pickerHsv,
                    pointer.pointerOnCanvas);

            if (takenColor.has_value())
            {
                inkPicker.pickerHsv = *takenColor;
                recolorInk(colorOf(inkPicker.pickerHsv));
                inkPicker.hexText = getColorToHex(
                    document.map.paletteColors.at(*inkPicker.editingInk));
            }
        }

        if (stroke.active && stroke.selectedTile.has_value())
        {
            const auto editedTileValue = getEditedTile(
                document.map, chosenLayer, stroke, assignMode);
            const auto pixel = tile::pixelAt(
                editedTileValue,
                getInspectedTileRect(sheetView.getFrameRect(), editedTileValue),
                pointer.pointerOnCanvas);

            if (pixel.has_value())
            {
                tile::paintLine(
                    atlasSheets.sheet(editedTileValue.atlas),
                    editedTileValue,
                    stroke.brushAtCell.value_or(*pixel),
                    *pixel,
                    stroke.erases
                        ? antwika::gfx::Color{.alpha = 0}
                        : document.map.paletteColors.at(inkPicker.activeInk));
                stroke.brushAtCell = pixel;
                atlasSheets.touch();
            }
        }

        if (auto *view = viewNow(); view != nullptr)
        {
            view->trackPointer(viewContextNow());
        }

        if (cameraRig.panning && viewChoice.activeView == map::View::Atlases)
        {
            const auto was =
                viewportRenderer.getViewport().toCanvas(
                    antwika::gfx::Point{
                        .x = pointer.lastPointerPosition.x,
                        .y = pointer.lastPointerPosition.y});

            sheetView.panPoint = antwika::gfx::PointF{
                sheetView.panPoint.x + pointer.pointerOnCanvas.x
                    - static_cast<float>(was.x),
                sheetView.panPoint.y + pointer.pointerOnCanvas.y
                    - static_cast<float>(was.y)};
            pointer.lastPointerPosition = movedEvent.position;
        }
        else if (cameraRig.panning && cameraRig.panGripPosition.has_value())
        {
            const auto hit = voxelmap::getPlaneHit(
                voxelmap::getRayInModelSpace(
                    voxelmap::getRayThrough(
                        getWorldCamera(play, cameraRig),
                        camera::kCanvasSize,
                        pointer.pointerOnCanvas),
                    getWorldRotation(play)),
                cameraRig.panGripPosition->y);

            if (hit.has_value())
            {
                cameraRig.view.transform.position += antwika::gfx::Vec3{
                    getWorldRotation(play)
                    * antwika::gfx::Vec4{
                        *cameraRig.panGripPosition - *hit, 0.0F}};
            }

            pointer.lastPointerPosition = movedEvent.position;
        }
        else if (cameraRig.panning)
        {
            const auto was = viewportRenderer.getViewport().toCanvas(
                antwika::gfx::Point{
                    .x = pointer.lastPointerPosition.x,
                    .y = pointer.lastPointerPosition.y});

            cameraRig.view.transform = camera::getPannedTransform(
                cameraRig.view.transform,
                static_cast<float>(was.x) - pointer.pointerOnCanvas.x,
                pointer.pointerOnCanvas.y - static_cast<float>(was.y),
                cameraRig.viewHeight
                    / static_cast<float>(camera::kCanvasSize.height));
            pointer.lastPointerPosition = movedEvent.position;
        }

        carryLamp();

        if (worldView.worldPaint.dragButton.has_value()
            && !worldView.worldPaint.shapeFromPosition.has_value()
            && isWorldShown())
        {
            const auto cell = voxelmap::getCellUnder(
                getWorldCamera(play, cameraRig),
                getWorldRotation(play),
                camera::kCanvasSize,
                pointer.pointerOnCanvas,
                antwika::voxel::getCubeTop(worldView.worldEdit.editLevel));

            if (cell.has_value() && cell != worldView.worldPaint.lastPaintedPosition)
            {
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
                worldView.worldPaint.lastPaintedPosition = cell;
                remesh.pending = true;
            }
        }

        if (isWorldShown())
        {
            cameraRig.dragOrbit(
                movedEvent.position, pointer.lastPointerPosition);
        }

        if (cameraRig.freeLook)
        {
            cameraRig.view.transform = camera::getRotatedTransform(
                cameraRig.view.transform,
                static_cast<float>(
                    movedEvent.position.x - pointer.lastPointerPosition.x)
                    * camera::kMouseTurn,
                static_cast<float>(
                    pointer.lastPointerPosition.y - movedEvent.position.y)
                    * camera::kMouseTurn);
        }

        pointer.lastPointerPosition = movedEvent.position;
        return;
    }

    void Editor::onPointerReleased(
        const input::PointerButtonReleased &upReleased)
    {
        if (upReleased.button == input::MouseButton::Middle)
        {
            cameraRig.panning = false;
            turningPlayer = false;
            cameraRig.panGripPosition.reset();
        }

        if (upReleased.button == input::MouseButton::Right
            && cameraRig.orbitFromPosition.has_value())
        {
            if (!cameraRig.orbiting && isWorldShown()
                && !play.playing)
            {
                rightTaken(upReleased.position);
            }

            cameraRig.orbitFromPosition.reset();
            cameraRig.orbiting = false;
        }

        if (upReleased.button == input::MouseButton::Left)
        {
            pointer.pointerHeld = false;
        }

        if (upReleased.button == input::MouseButton::Left)
        {
            if (const auto notice = plan.letGo(); notice.has_value())
            {
                showStatus(*notice, true, 120);
            }
        }

        if (upReleased.button == input::MouseButton::Left)
        {
            endSliderDrag();
        }

        if (upReleased.button == input::MouseButton::Left)
        {
            characterView.mark.selecting = false;
            characterView.mark.draggingPatch = false;
            characterView.mark.grabbedMarkSelection.reset();
            characterView.mark.grabbedAtCell.reset();
        }

        if (upReleased.button == input::MouseButton::Left
            || (stroke.erases
                && upReleased.button
                       == input::MouseButton::Right))
        {
            stroke.active = false;
            stroke.erases = false;
            inkPicker.pickerDragging = false;
            stroke.brushAtCell.reset();
        }

        finishShape(upReleased.button);
        finishStamp(upReleased.button);

        if (worldView.worldPaint.draggedLamp.has_value()
            && upReleased.button == input::MouseButton::Left)
        {
            worldView.worldPaint.draggedLamp.reset();
        }

        if (worldView.worldPaint.dragButton.has_value()
            && upReleased.button == *worldView.worldPaint.dragButton)
        {
            worldView.worldPaint.dragButton.reset();
            worldView.worldPaint.lastPaintedPosition.reset();
        }

        if (upReleased.button == input::MouseButton::Left
            && stroke.lineFromCell.has_value()
            && stroke.selectedTile.has_value())
        {
            const auto projectToScreen =
                viewportRenderer.getViewport().toCanvas(
                    antwika::gfx::Point{
                        .x = upReleased.position.x,
                        .y = upReleased.position.y});
            const auto editedTileValue = getEditedTile(
                document.map, chosenLayer, stroke, assignMode);
            const auto pixel = tile::pixelAt(
                editedTileValue,
                getInspectedTileRect(sheetView.getFrameRect(), editedTileValue),
                antwika::gfx::PointF{
                    static_cast<float>(projectToScreen.x),
                    static_cast<float>(projectToScreen.y)});

            if (pixel.has_value() && !atlasView.blockedAsTransitionSlot(viewContextNow()))
            {
                auto &sheet =
                    atlasSheets.sheet(editedTileValue.atlas);
                const auto ink =
                    document.map.paletteColors.at(inkPicker.activeInk);

                pushUndo();

                if (preferences.paint == map::Paint::Rect)
                {
                    tile::paintPixels(
                        sheet,
                        editedTileValue,
                        tile::getRectPixels(
                            *stroke.lineFromCell, *pixel),
                        ink);
                }
                else if (preferences.paint == map::Paint::Circle)
                {
                    tile::paintPixels(
                        sheet,
                        editedTileValue,
                        tile::getCirclePixels(
                            *stroke.lineFromCell, *pixel),
                        ink);
                }
                else
                {
                    tile::paintLine(
                        sheet,
                        editedTileValue,
                        *stroke.lineFromCell,
                        *pixel,
                        ink);
                }

                atlasSheets.touch();
            }

            stroke.lineFromCell.reset();
        }

        if (upReleased.button == input::MouseButton::Left
            && isWorldShown())
        {
            if (stroke.doubleClickAtPoint.has_value())
            {
                const auto pickedFace = voxelmap::getTilePicked(
                    visibleCells(),
                    worldMeshes.getFaces(),
                    worldMeshes.getDrawnAs(),
                    getWorldCamera(play, cameraRig),
                    getWorldRotation(play),
                    camera::kCanvasSize,
                    *stroke.doubleClickAtPoint);

                if (pickedFace.has_value())
                {
                    stroke.selectedTile = pickedFace;
                    stroke.selectedEdges.reset();
                    stroke.dragFromCell.reset();
                    stroke.dragFromPoint.reset();
                    viewChoice.activeView = map::View::Atlases;
                }

                stroke.doubleClickAtPoint.reset();
            }
        }
        else if (upReleased.button == input::MouseButton::Left
                 && viewChoice.activeView == map::View::Atlases)
        {
            const auto projectToScreen =
                viewportRenderer.getViewport().toCanvas(
                    antwika::gfx::Point{
                        .x = upReleased.position.x,
                        .y = upReleased.position.y});
            const antwika::gfx::PointF releasedAtPoint{
                static_cast<float>(projectToScreen.x),
                static_cast<float>(projectToScreen.y)};
            const auto gesture = gestureFrom(
                document.map.tilemap,
                sheetView.getFrameRect(),
                sheetView.getGridRect(document.map.tilemap),
                sheetView.getClipRect(),
                stroke.dragFromPoint,
                releasedAtPoint,
                stroke.selectedTile.has_value(),
                stroke.selectedEdges);

            switch (gesture.action)
            {
            case PointerAction::Swap:
                pushUndo();

                if (getHeldModifiers().control)
                {
                    duplicateTile(
                        gesture.fromCell, gesture.toCell);

                    break;
                }

                tilemap::swapTiles(
                    document.map.tilemap,
                    gesture.fromCell,
                    gesture.toCell);
                break;
            case PointerAction::Look:
            {
                auto tile = document.map.tilemap.getEntryAt(
                    gesture.toCell.column, gesture.toCell.row);

                if (tile.has_value()
                    && consumeAssignClick(*tile))
                {
                    break;
                }

                if (!tile.has_value())
                {
                    tile = tilemap::suggestedTileFor(
                        document.map.tilemap, gesture.toCell);

                    if (tile.has_value())
                    {
                        pushUndo();
                        tilemap::putTile(
                            document.map.tilemap,
                            gesture.toCell,
                            *tile);
                        wipeTile(*tile);
                    }
                }

                if (tile.has_value())
                {
                    stroke.selectedTile = tile;
                }

                break;
            }
            case PointerAction::Rule:
            {
                const auto tile = document.map.tilemap.getEntryAt(
                    gesture.toCell.column, gesture.toCell.row);

                if (tile.has_value()
                    && !blockedAsVariant())
                {
                    const auto forbidden =
                        !stroke.allows(getActiveRules(document.map, chosenLayer), *tile);

                    pushUndo();

                    for (const auto edge :
                         edgesIn(*stroke.selectedEdges))
                    {
                        getActiveRules(document.map, chosenLayer).setAllows(
                            *stroke.selectedTile,
                            edge,
                            *tile,
                            forbidden);
                    }

                    rebuildWorld();
                }

                break;
            }
            case PointerAction::Turn:
            {
                if (blockedAsVariant())
                {
                    break;
                }

                const auto cornerState = getActiveRules(document.map, chosenLayer).getCorner(
                    *stroke.selectedTile, gesture.corner);

                pushUndo();
                getActiveRules(document.map, chosenLayer).setCorner(
                    *stroke.selectedTile,
                    gesture.corner,
                    !cornerState.has_value()
                        ? std::optional{true}
                    : *cornerState ? std::optional{false}
                            : std::nullopt);
                break;
            }
            case PointerAction::PixelSelection:
                stroke.selectedEdges =
                    stroke.selectedEdges == gesture.selection
                                   ? std::nullopt
                                   : std::optional{gesture.selection};
                break;
            case PointerAction::Nothing:
                break;
            }

            stroke.dragFromCell.reset();
            stroke.dragFromPoint.reset();
        }

        return;
    }

}
