#include <antwika/editor/ui/AtlasView.hpp>
#include <antwika/editor/ui/CharacterView.hpp>
#include <antwika/editor/ui/EditorLook.hpp>
#include <antwika/editor/ui/IconSheet.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/gfx/PointF.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/voxelmap/VoxelPick.hpp>

#include "antwika/editor/Editor.hpp"

namespace antwika::editor
{

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

        if (inkPanel.inkPicker.pickerDragging && inkPanel.inkPicker.editingInk.has_value())
        {
            const auto takenColor =
                getColorAtPoint(
                    camera::kCanvasSize,
                    getRailWidthOnCanvas(),
                    inkPanel.inkPicker.pickerHsv,
                    pointer.pointerOnCanvas);

            if (takenColor.has_value())
            {
                inkPanel.inkPicker.pickerHsv = *takenColor;
                inkPanel.recolorInk(colorOf(inkPanel.inkPicker.pickerHsv));
                inkPanel.inkPicker.hexText = getColorToHex(
                    document.map.paletteColors.at(*inkPanel.inkPicker.editingInk));
            }
        }

        if (auto *view = viewNow(); view != nullptr)
        {
            view->trackPointer(viewContextNow());
        }

        carryEntity();

        if (cameraRig.panning && cameraRig.panGripPosition.has_value())
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
        else if (cameraRig.panning
                 && pointer.lastPointerPosition != movedEvent.position)
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
            endSliderDrag();
            endEdgeDrag();
        }

        if (upReleased.button == input::MouseButton::Left
            || (stroke.erases
                && upReleased.button
                       == input::MouseButton::Right))
        {
            stroke.active = false;
            stroke.erases = false;
            inkPanel.inkPicker.pickerDragging = false;
            stroke.brushAtCell.reset();
        }

        worldView.finishShape(viewContextNow(), upReleased.button);
        worldView.finishStamp(viewContextNow(), upReleased.button);
        worldView.endPaintDrag(upReleased.button);

        if (upReleased.button == input::MouseButton::Left)
        {
            entityPick.dragging = false;
            entityPick.dragUndoKept = false;
        }

        if (auto *view = viewNow(); view != nullptr)
        {
            static_cast<void>(
                view->consumeRelease(viewContextNow(), upReleased));
        }

        if (upReleased.button == input::MouseButton::Left
            && isWorldShown() && stroke.doubleClickAtPoint.has_value())
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
                viewChoice.activeView = View::Atlases;
            }

            stroke.doubleClickAtPoint.reset();
        }

        return;
    }

}
