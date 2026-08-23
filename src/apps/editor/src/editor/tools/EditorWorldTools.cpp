#include <antwika/input/MouseButton.hpp>
#include <antwika/light/PointLight.hpp>
#include <antwika/rules/Gates.hpp>
#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/voxelmap/VoxelPick.hpp>

#include "antwika/editor/Editor.hpp"

namespace antwika::editor
{

    std::vector<voxel::VoxelPosition> Editor::getShapedCubes(
        const voxel::VoxelPosition fromPosition,
        const voxel::VoxelPosition toPosition) const
    {
        const auto a = antwika::voxel::cubeCornerOf(fromPosition);
        const auto b = antwika::voxel::cubeCornerOf(toPosition);

        std::vector<voxel::VoxelPosition> positions;

        if (settings.paint == map::Paint::Rect)
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
        if (settings.tool != map::Tool::Brush
            || (settings.paint != map::Paint::Rect
                && settings.paint != map::Paint::Line))
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

        const auto position = voxelmap::getCellUnder(
            worldCamera(),
            worldRotation(),
            camera::kCanvasSize,
            pointer.pointerOnCanvas,
            antwika::voxel::getCubeTop(editLevel));

        if (position.has_value())
        {
            pushUndo();

            for (const auto cube :
                 getShapedCubes(*shapeFromPosition, *position))
            {
                document.map.voxels = voxel::getWithRampsRebuilt(
                    dragPaintButton == input::MouseButton::Left
                                     ? voxel::withBlockAt(
                              document.map.voxels,
                              cube,
                              settings.kind,
                              rampFacing)
                        : voxel::withoutBlockAt(
                              document.map.voxels, cube),
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

        auto &landing = settings.tool == map::Tool::Start
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

        const auto position = voxelmap::getCellUnder(
            worldCamera(),
            worldRotation(),
            camera::kCanvasSize,
            pointer.pointerOnCanvas,
            antwika::voxel::getCubeTop(editLevel));

        if (position.has_value() && *position != draggedLamp->position)
        {
            document.map.lamps = light::withLampAt(
                light::withoutLampAt(document.map.lamps, draggedLamp->position),
                *position,
                draggedLamp->tintColor);
            draggedLamp->position = *position;
            lightPasses.forget();
        }
    }

}
