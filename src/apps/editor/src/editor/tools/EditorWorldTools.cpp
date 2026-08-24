#include <antwika/input/MouseButton.hpp>
#include <antwika/light/PointLight.hpp>
#include <antwika/rules/Gates.hpp>
#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/voxelmap/VoxelPick.hpp>

#include "antwika/editor/tools/ShapedCubes.hpp"
#include "antwika/editor/Editor.hpp"

namespace antwika::editor
{

    bool Editor::beginShape(
        const voxel::VoxelPosition position,
        const input::MouseButton button)
    {
        if (preferences.tool != map::Tool::Brush
            || (preferences.paint != map::Paint::Rect
                && preferences.paint != map::Paint::Line))
        {
            return false;
        }

        worldView.worldPaint.shapeFromPosition = position;
        worldView.worldPaint.dragButton = button;

        return true;
    }

    void Editor::finishShape(const input::MouseButton button)
    {
        if (!worldView.worldPaint.shapeFromPosition.has_value()
            || !worldView.worldPaint.dragButton.has_value()
            || button != *worldView.worldPaint.dragButton)
        {
            return;
        }

        const auto position = voxelmap::getCellUnder(
            getWorldCamera(play, cameraRig),
            getWorldRotation(play),
            camera::kCanvasSize,
            pointer.pointerOnCanvas,
            antwika::voxel::getCubeTop(worldView.worldEdit.editLevel));

        if (position.has_value())
        {
            pushUndo();

            for (const auto cube :
                 getShapedCubes(
                     *worldView.worldPaint.shapeFromPosition,
                     *position,
                     preferences.paint))
            {
                document.map.voxels = voxel::getWithRampsRebuilt(
                    worldView.worldPaint.dragButton == input::MouseButton::Left
                                     ? voxel::withBlockAt(
                              document.map.voxels,
                              cube,
                              preferences.kind,
                              rampFacing)
                        : voxel::withoutBlockAt(
                              document.map.voxels, cube),
                    cube);
            }

            rebuildWorld();
        }

        worldView.worldPaint.shapeFromPosition.reset();
    }

    void Editor::placeStartOrExit(
        const voxel::VoxelPosition position, const input::MouseButton button)
    {
        pushUndo();

        auto &landing = preferences.tool == map::Tool::Start
                      ? document.map.spawnCubePosition
                      : document.map.exitCubePosition;

        landing =
            button == input::MouseButton::Left
                    ? std::optional{position}
                    : std::nullopt;

        if (button == input::MouseButton::Left
            && !rules::isCubeOccupied(
                document.map.voxels, antwika::voxel::cubeCornerOf(position)))
        {
            document.map.voxels = voxel::getWithRampsRebuilt(
                voxel::withBlockAt(document.map.voxels, position), position);
            rebuildWorld();
        }
    }

    bool Editor::beginLampCarry(const voxel::VoxelPosition position)
    {
        for (const auto &lamp : document.map.lamps)
        {
            if (lamp.position == position)
            {
                pushUndo();
                worldView.worldPaint.draggedLamp = lamp;

                return true;
            }
        }

        return false;
    }

    void Editor::carryLamp()
    {
        if (!worldView.worldPaint.draggedLamp.has_value() || !isWorldShown())
        {
            return;
        }

        const auto position = voxelmap::getCellUnder(
            getWorldCamera(play, cameraRig),
            getWorldRotation(play),
            camera::kCanvasSize,
            pointer.pointerOnCanvas,
            antwika::voxel::getCubeTop(worldView.worldEdit.editLevel));

        if (position.has_value() && *position != worldView.worldPaint.draggedLamp->position)
        {
            document.map.lamps = light::withLampAt(
                light::withoutLampAt(
                    document.map.lamps,
                    worldView.worldPaint.draggedLamp->position),
                *position,
                worldView.worldPaint.draggedLamp->tintColor);
            worldView.worldPaint.draggedLamp->position = *position;
            lightPasses.forget();
        }
    }

}
