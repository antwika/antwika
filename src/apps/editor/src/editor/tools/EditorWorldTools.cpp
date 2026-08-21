#include <antwika/input/MouseButton.hpp>
#include <antwika/light/PointLight.hpp>
#include <antwika/rules/Gates.hpp>
#include <antwika/voxel/VoxelCell.hpp>
#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/voxelmap/VoxelPick.hpp>

#include "antwika/editor/Editor.hpp"

namespace antwika::editor
{

    std::vector<voxel::VoxelCell> Editor::shapedCubes(
        const voxel::VoxelCell fromCell, const voxel::VoxelCell toCell) const
    {
        const auto a = antwika::voxel::cubeCornerOf(fromCell);
        const auto b = antwika::voxel::cubeCornerOf(toCell);

        std::vector<voxel::VoxelCell> cells;

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
                    cells.push_back(
                        voxel::VoxelCell{.x = x, .y = a.y, .z = z});
                }
            }

            return cells;
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

            cells.push_back(
                voxel::VoxelCell{
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

        return cells;
    } // GCOVR_EXCL_LINE

    bool Editor::beginShape(
        const voxel::VoxelCell cell,
        const input::MouseButton button)
    {
        if (tool != map::Tool::Brush
            || (paintMode != map::Paint::Rect
                && paintMode != map::Paint::Line))
        {
            return false;
        }

        shapeFromCell = cell;
        dragPaintButton = button;

        return true;
    }

    void Editor::finishShape(const input::MouseButton button)
    {
        if (!shapeFromCell.has_value() || !dragPaintButton.has_value()
            || button != *dragPaintButton)
        {
            return;
        }

        const auto cell = voxelmap::cellUnder(
            worldCamera(),
            worldRotation(),
            camera::kCanvasSize,
            pointer.pointerOnCanvas,
            antwika::voxel::cubeTop(editLevel));

        if (cell.has_value())
        {
            pushUndo();

            for (const auto cube :
                 shapedCubes(*shapeFromCell, *cell))
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

        shapeFromCell.reset();
    }

    void Editor::placeStartOrExit(
        const voxel::VoxelCell cell, const input::MouseButton button)
    {
        pushUndo();

        auto &landing = tool == map::Tool::Start
                      ? map.spawnCubeCell
                      : map.exitCubeCell;

        landing =
            button == input::MouseButton::Left
                    ? std::optional{cell}
                    : std::nullopt;

        if (button == input::MouseButton::Left
            && !rules::cubeOccupied(
                map.voxels, antwika::voxel::cubeCornerOf(cell)))
        {
            map.voxels = voxel::withRampsRebuilt(
                voxel::withBlockAt(map.voxels, cell), cell);
            rebuildWorld();
        }
    }

    bool Editor::beginLampCarry(const voxel::VoxelCell cell)
    {
        for (const auto &lamp : map.lamps)
        {
            if (lamp.cell == cell)
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

        const auto cell = voxelmap::cellUnder(
            worldCamera(),
            worldRotation(),
            camera::kCanvasSize,
            pointer.pointerOnCanvas,
            antwika::voxel::cubeTop(editLevel));

        if (cell.has_value() && *cell != draggedLamp->cell)
        {
            map.lamps = light::withLampAt(
                light::withoutLampAt(map.lamps, draggedLamp->cell),
                *cell,
                draggedLamp->tintColor);
            draggedLamp->cell = *cell;
            lightPasses.forget();
        }
    }

}
