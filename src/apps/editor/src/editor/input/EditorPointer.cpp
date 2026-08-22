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
            viewportRenderer.viewport().toCanvas(
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

        if (playing)
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

        if (activeView == map::View::Character)
        {
            const auto characterCell =
                characterAt(camera::kCanvasSize, pointer.pointerOnCanvas);

            characterView.mark.hoveredWayRow =
                characterCell.has_value()
                    ? std::optional<std::size_t>{
                          *characterCell / character::kCharacterFrames}
                    : std::nullopt;
        }

        if (inkPicker.pickerDragging && inkPicker.editingInk.has_value())
        {
            const auto takenColor =
                colorAtPoint(camera::kCanvasSize, inkPicker.pickerHsv,
                    pointer.pointerOnCanvas);

            if (takenColor.has_value())
            {
                inkPicker.pickerHsv = *takenColor;
                recolorInk(colorOf(inkPicker.pickerHsv));
                inkPicker.hexText = colorToHex(
                    map.paletteColors.at(*inkPicker.editingInk));
            }
        }

        if (strokeActive && selectedTile.has_value())
        {
            const auto editedTileValue = editedTile();
            const auto pixel = tile::pixelAt(
                editedTileValue,
                inspectedTileRect(frameRect(), editedTileValue),
                pointer.pointerOnCanvas);

            if (pixel.has_value())
            {
                tile::paintLine(
                    atlasSheets.sheet(editedTileValue.atlas),
                    editedTileValue,
                    brushAtCell.value_or(*pixel),
                    *pixel,
                    strokeErases
                        ? antwika::gfx::Color{.alpha = 0}
                        : map.paletteColors.at(inkPicker.activeInk));
                brushAtCell = pixel;
                atlasSheets.touch();
            }
        }

        if ((characterView.mark.selecting || characterView.mark.draggingPatch)
            && activeView == map::View::Character)
        {
            const auto pixel = character::characterPixelAt(
                characterCanvasRect(camera::kCanvasSize),
                pointer.pointerOnCanvas);

            if (pixel.has_value() && characterView.mark.selecting
                && characterView.mark.selection.has_value())
            {
                characterView.mark.selection->toCell = *pixel;
            }

            if (pixel.has_value() && characterView.mark.draggingPatch
                && characterView.mark.grabbedMarkSelection.has_value()
                && characterView.mark.grabbedAtCell.has_value())
            {
                characterView.mark.selection = character::movedSelection(
                    *characterView.mark.grabbedMarkSelection,
                    static_cast<std::int32_t>(
                        pixel->column)
                        - static_cast<std::int32_t>(
                            characterView.mark.grabbedAtCell->column),
                    static_cast<std::int32_t>(
                        pixel->row)
                        - static_cast<std::int32_t>(
                            characterView.mark.grabbedAtCell->row));
            }
        }

        if (strokeActive && activeView == map::View::Character
            && characterView.mark.selectedFrame.has_value())
        {
            const auto pixel = character::characterPixelAt(
                characterCanvasRect(camera::kCanvasSize),
                pointer.pointerOnCanvas);

            if (pixel.has_value())
            {
                character::paintCharacterLine(
                    characterView.sheet(),
                    *characterView.mark.selectedFrame
                        / character::kCharacterFrames,
                    *characterView.mark.selectedFrame
                        % character::kCharacterFrames,
                    brushAtCell.value_or(*pixel),
                    *pixel,
                    character::characterPaletteColor(
                        map.paletteColors,
                        strokeErases ? character::kTransparentInk
                                     : inkPicker.activeInk));
                brushAtCell = pixel;
                characterView.touch();
            }
        }

        if (strokeActive && activeView == map::View::Icons
            && iconsView.picked().has_value())
        {
            const auto pixel = antwika::editor::iconPixelAt(
                antwika::editor::editedIconRect(camera::kCanvasSize),
                pointer.pointerOnCanvas);

            if (pixel.has_value())
            {
                iconsView.paint(viewportRenderer, *pixel, strokeErases);
                brushAtCell = pixel;
            }
        }

        if (panning && activeView == map::View::Atlases)
        {
            const auto was =
                viewportRenderer.viewport().toCanvas(
                    antwika::gfx::Point{
                        .x = pointer.lastPointerPosition.x,
                        .y = pointer.lastPointerPosition.y});

            gridPanPoint = antwika::gfx::PointF{
                gridPanPoint.x + pointer.pointerOnCanvas.x
                    - static_cast<float>(was.x),
                gridPanPoint.y + pointer.pointerOnCanvas.y
                    - static_cast<float>(was.y)};
            pointer.lastPointerPosition = movedEvent.position;
        }
        else if (panning && panGripPosition.has_value())
        {
            const auto hit = voxelmap::planeHit(
                voxelmap::rayInModelSpace(
                    voxelmap::rayThrough(
                        worldCamera(),
                        camera::kCanvasSize,
                        pointer.pointerOnCanvas),
                    worldRotation()),
                panGripPosition->y);

            if (hit.has_value())
            {
                cameraView.transform.position += antwika::gfx::Vec3{
                    worldRotation()
                    * antwika::gfx::Vec4{
                        *panGripPosition - *hit, 0.0F}};
            }

            pointer.lastPointerPosition = movedEvent.position;
        }
        else if (panning)
        {
            const auto was = viewportRenderer.viewport().toCanvas(
                antwika::gfx::Point{
                    .x = pointer.lastPointerPosition.x,
                    .y = pointer.lastPointerPosition.y});

            cameraView.transform = camera::panned(
                cameraView.transform,
                static_cast<float>(was.x) - pointer.pointerOnCanvas.x,
                pointer.pointerOnCanvas.y - static_cast<float>(was.y),
                viewHeight
                    / static_cast<float>(camera::kCanvasSize.height));
            pointer.lastPointerPosition = movedEvent.position;
        }

        carryLamp();

        if (dragPaintButton.has_value() && !shapeFromPosition.has_value()
            && activeView == map::View::World)
        {
            const auto cell = voxelmap::cellUnder(
                worldCamera(),
                worldRotation(),
                camera::kCanvasSize,
                pointer.pointerOnCanvas,
                antwika::voxel::cubeTop(editLevel));

            if (cell.has_value() && cell != lastPaintedPosition)
            {
                map.voxels = voxel::withRampsRebuilt(
                    tool == map::Tool::Eraser
                          ? voxel::withoutBlockAt(
                              map.voxels, *cell)
                        : voxel::withBlockAt(
                              map.voxels,
                              *cell,
                              brushKind,
                              rampFacing),
                    *cell);
                lastPaintedPosition = cell;
                remeshPending = true;
            }
        }

        if (orbitFromPosition.has_value()
            && activeView == map::View::World)
        {
            if (!orbiting
                && std::abs(
                       movedEvent.position.x - orbitFromPosition->x)
                           + std::abs(
                               movedEvent.position.y
                               - orbitFromPosition->y)
                       > 4)
            {
                orbiting = true;
            }

            if (orbiting)
            {
                orbitCamera(
                    static_cast<float>(
                        movedEvent.position.x - pointer.lastPointerPosition.x)
                        * camera::kMouseTurn,
                    static_cast<float>(
                        pointer.lastPointerPosition.y - movedEvent.position.y)
                        * camera::kMouseTurn);
            }
        }

        if (freeLook)
        {
            cameraView.transform = camera::rotated(
                cameraView.transform,
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
            panning = false;
            turningPlayer = false;
            panGripPosition.reset();
        }

        if (upReleased.button == input::MouseButton::Right
            && orbitFromPosition.has_value())
        {
            if (!orbiting && activeView == map::View::World
                && !playing)
            {
                rightTaken(upReleased.position);
            }

            orbitFromPosition.reset();
            orbiting = false;
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
            || (strokeErases
                && upReleased.button
                       == input::MouseButton::Right))
        {
            strokeActive = false;
            strokeErases = false;
            inkPicker.pickerDragging = false;
            brushAtCell.reset();
        }

        finishShape(upReleased.button);
        finishStamp(upReleased.button);

        if (draggedLamp.has_value()
            && upReleased.button == input::MouseButton::Left)
        {
            draggedLamp.reset();
        }

        if (dragPaintButton.has_value()
            && upReleased.button == *dragPaintButton)
        {
            dragPaintButton.reset();
            lastPaintedPosition.reset();
        }

        if (upReleased.button == input::MouseButton::Left
            && lineFromCell.has_value()
            && selectedTile.has_value())
        {
            const auto projectToScreen =
                viewportRenderer.viewport().toCanvas(
                    antwika::gfx::Point{
                        .x = upReleased.position.x,
                        .y = upReleased.position.y});
            const auto editedTileValue = editedTile();
            const auto pixel = tile::pixelAt(
                editedTileValue,
                inspectedTileRect(frameRect(), editedTileValue),
                antwika::gfx::PointF{
                    static_cast<float>(projectToScreen.x),
                    static_cast<float>(projectToScreen.y)});

            if (pixel.has_value() && !blockedAsTransitionSlot())
            {
                auto &sheet =
                    atlasSheets.sheet(editedTileValue.atlas);
                const auto ink =
                    map.paletteColors.at(inkPicker.activeInk);

                pushUndo();

                if (paintMode == map::Paint::Rect)
                {
                    tile::paintPixels(
                        sheet,
                        editedTileValue,
                        tile::rectPixels(
                            *lineFromCell, *pixel),
                        ink);
                }
                else if (paintMode == map::Paint::Circle)
                {
                    tile::paintPixels(
                        sheet,
                        editedTileValue,
                        tile::circlePixels(
                            *lineFromCell, *pixel),
                        ink);
                }
                else
                {
                    tile::paintLine(
                        sheet,
                        editedTileValue,
                        *lineFromCell,
                        *pixel,
                        ink);
                }

                atlasSheets.touch();
            }

            lineFromCell.reset();
        }

        if (upReleased.button == input::MouseButton::Left
            && activeView == map::View::World)
        {
            if (doubleClickAtPoint.has_value())
            {
                const auto pickedFace = voxelmap::tilePicked(
                    visibleCells(),
                    worldMeshes.faces(),
                    worldMeshes.drawnAs(),
                    worldCamera(),
                    worldRotation(),
                    camera::kCanvasSize,
                    *doubleClickAtPoint);

                if (pickedFace.has_value())
                {
                    selectedTile = pickedFace;
                    selectedEdges.reset();
                    dragFromCell.reset();
                    dragFromPoint.reset();
                    activeView = map::View::Atlases;
                }

                doubleClickAtPoint.reset();
            }
        }
        else if (upReleased.button == input::MouseButton::Left
                 && activeView == map::View::Atlases)
        {
            const auto projectToScreen =
                viewportRenderer.viewport().toCanvas(
                    antwika::gfx::Point{
                        .x = upReleased.position.x,
                        .y = upReleased.position.y});
            const antwika::gfx::PointF releasedAtPoint{
                static_cast<float>(projectToScreen.x),
                static_cast<float>(projectToScreen.y)};
            const auto gesture = gestureFrom(
                map.tilemap,
                frameRect(),
                gridRect(),
                sheetClipRect(),
                dragFromPoint,
                releasedAtPoint,
                selectedTile.has_value(),
                selectedEdges);

            switch (gesture.action)
            {
            case PointerAction::Swap:
                pushUndo();

                if (heldModifiers().control)
                {
                    duplicateTile(
                        gesture.fromCell, gesture.toCell);

                    break;
                }

                tilemap::swapTiles(
                    map.tilemap,
                    gesture.fromCell,
                    gesture.toCell);
                break;
            case PointerAction::Look:
            {
                auto tile = map.tilemap.at(
                    gesture.toCell.column, gesture.toCell.row);

                if (tile.has_value()
                    && handleAssignClick(*tile))
                {
                    break;
                }

                if (!tile.has_value())
                {
                    tile = tilemap::suggestedTileFor(
                        map.tilemap, gesture.toCell);

                    if (tile.has_value())
                    {
                        pushUndo();
                        tilemap::putTile(
                            map.tilemap,
                            gesture.toCell,
                            *tile);
                        wipeTile(*tile);
                    }
                }

                if (tile.has_value())
                {
                    selectedTile = tile;
                }

                break;
            }
            case PointerAction::Rule:
            {
                const auto tile = map.tilemap.at(
                    gesture.toCell.column, gesture.toCell.row);

                if (tile.has_value()
                    && !blockedAsVariant())
                {
                    const auto forbidden =
                        !selectionAllows(*tile);

                    pushUndo();

                    for (const auto edge :
                         edgesIn(*selectedEdges))
                    {
                        activeRules().setAllows(
                            *selectedTile,
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

                const auto cornerState = activeRules().corner(
                    *selectedTile, gesture.corner);

                pushUndo();
                activeRules().setCorner(
                    *selectedTile,
                    gesture.corner,
                    !cornerState.has_value()
                        ? std::optional{true}
                    : *cornerState ? std::optional{false}
                            : std::nullopt);
                break;
            }
            case PointerAction::PixelSelection:
                selectedEdges =
                    selectedEdges == gesture.selection
                                   ? std::nullopt
                                   : std::optional{gesture.selection};
                break;
            case PointerAction::Nothing:
                break;
            }

            dragFromCell.reset();
            dragFromPoint.reset();
        }

        return;
    }

}
