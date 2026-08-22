#include <antwika/input/MouseButton.hpp>
#include <antwika/light/PointLight.hpp>
#include <antwika/rules/Gates.hpp>
#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/voxelmap/VoxelPick.hpp>

#include "antwika/editor/Editor.hpp"

namespace antwika::editor
{

    std::vector<voxel::VoxelPosition> Editor::shapedCubes(
        const voxel::VoxelPosition fromPosition,
        const voxel::VoxelPosition toPosition) const
    {
        const auto a = antwika::voxel::cubeCornerOf(fromPosition);
        const auto b = antwika::voxel::cubeCornerOf(toPosition);

        std::vector<voxel::VoxelPosition> positions;

        if (paintMode == map::Paint::Rect)
        {
            for (auto x = std::min(a.x, b.x);
                 x <= std::max(a.x, b.x);
                 x += voxel::kCubeSide)
            {
                for (auto z = std::min(a.z, b.z);
                     z <= std::max(a.z, b.z);
                     z += voxel::kCubeSide)
                {
                    positions.push_back(
                        voxel::VoxelPosition{.x = x, .y = a.y, .z = z});
                }
            }

            return positions;
        }

        const auto deltaX = (b.x - a.x) / voxel::kCubeSide;
        const auto alongSpan = (b.z - a.z) / voxel::kCubeSide;
        const auto steps =
            std::max(std::abs(deltaX), std::abs(alongSpan));

        for (std::int32_t step = 0; step <= steps; ++step)
        {
            const auto part =
                steps == 0
                       ? 0.0
                       : static_cast<double>(step)
                          / static_cast<double>(steps);

            positions.push_back(
                voxel::VoxelPosition{
                    .x = a.x
                         + (static_cast<std::int32_t>(
                                std::llround(
                                    part
                                    * static_cast<double>(
                                        deltaX)))
                            * voxel::kCubeSide),
                    .y = a.y,
                    .z = a.z
                         + (static_cast<std::int32_t>(
                                std::llround(
                                    part
                                    * static_cast<double>(
                                        alongSpan)))
                            * voxel::kCubeSide)});
        }

        return positions;
    } // GCOVR_EXCL_LINE

    bool Editor::beginShape(
        const voxel::VoxelPosition position,
        const input::MouseButton button)
    {
        if (tool != map::Tool::Brush
            || (paintMode != map::Paint::Rect
                && paintMode != map::Paint::Line))
        {
            return false;
        }

        shapeFromPosition = position;
        dragPaintButton = button;

        return true;
    }

    void Editor::finishShape(const input::MouseButton button)
    {
        if (!shapeFromPosition.has_value() || !dragPaintButton.has_value()
            || button != *dragPaintButton)
        {
            return;
        }

        const auto position = voxelmap::cellUnder(
            worldCamera(),
            worldRotation(),
            camera::kCanvasSize,
            pointer.pointerOnCanvas,
            antwika::voxel::cubeTop(editLevel));

        if (position.has_value())
        {
            pushUndo();

            for (const auto cube :
                 shapedCubes(*shapeFromPosition, *position))
            {
                map.voxels = voxel::withRampsRebuilt(
                    dragPaintButton == input::MouseButton::Left
                                     ? voxel::withBlockAt(
                              map.voxels,
                              cube,
                              brushKind,
                              rampFacing)
                        : voxel::withoutBlockAt(
                              map.voxels, cube),
                    cube);
            }

            rebuildWorld();
        }

        shapeFromPosition.reset();
    }

    void Editor::placeStartOrExit(
        const voxel::VoxelPosition position, const input::MouseButton button)
    {
        pushUndo();

        auto &landing = tool == map::Tool::Start
                      ? map.spawnCubePosition
                      : map.exitCubePosition;

        landing =
            button == input::MouseButton::Left
                    ? std::optional{position}
                    : std::nullopt;

        if (button == input::MouseButton::Left
            && !rules::cubeOccupied(
                map.voxels, antwika::voxel::cubeCornerOf(position)))
        {
            map.voxels = voxel::withRampsRebuilt(
                voxel::withBlockAt(map.voxels, position), position);
            rebuildWorld();
        }
    }

    bool Editor::beginLampCarry(const voxel::VoxelPosition position)
    {
        for (const auto &lamp : map.lamps)
        {
            if (lamp.position == position)
            {
                pushUndo();
                draggedLamp = lamp;

                return true;
            }
        }

        return false;
    }

    void Editor::carryLamp()
    {
        if (!draggedLamp.has_value() || activeView != map::View::World)
        {
            return;
        }

        const auto position = voxelmap::cellUnder(
            worldCamera(),
            worldRotation(),
            camera::kCanvasSize,
            pointer.pointerOnCanvas,
            antwika::voxel::cubeTop(editLevel));

        if (position.has_value() && *position != draggedLamp->position)
        {
            map.lamps = light::withLampAt(
                light::withoutLampAt(map.lamps, draggedLamp->position),
                *position,
                draggedLamp->tintColor);
            draggedLamp->position = *position;
            lightPasses.forget();
        }
    }

}
