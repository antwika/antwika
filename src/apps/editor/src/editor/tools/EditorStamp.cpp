#include <antwika/input/MouseButton.hpp>
#include <antwika/voxel/VoxelCell.hpp>
#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/voxelmap/VoxelPick.hpp>

#include "antwika/editor/Editor.hpp"

namespace antwika::editor
{

    void Editor::pressStamp(
        const voxel::VoxelCell cell, const input::MouseButton button)
    {
        if (button == input::MouseButton::Right)
        {
            stampCells.clear();
            stampFromCell.reset();

            return;
        }

        if (!stampCells.empty())
        {
            pushUndo();

            const auto stampCorner = antwika::voxel::cubeCornerOf(cell);

            for (const auto offset : stampCells)
            {
                const voxel::VoxelCell cornerCell{
                    .x = stampCorner.x + offset.x,
                    .y = offset.y,
                    .z = stampCorner.z + offset.z};

                map.voxels = voxel::withRampsRebuilt(
                    voxel::withBlockAt(
                        map.voxels,
                        cornerCell,
                        offset.kind,
                        offset.facing),
                    cornerCell);
            }

            rebuildWorld();

            return;
        }

        stampFromCell = cell;
    }

    void Editor::finishStamp(const input::MouseButton button)
    {
        if (!stampFromCell.has_value()
            || button != input::MouseButton::Left)
        {
            return;
        }

        const auto cell = voxelmap::cellUnder(
            worldCamera(),
            worldRotation(),
            camera::kCanvasSize,
            pointer.pointerOnCanvas,
            antwika::voxel::cubeTop(editLevel));

        if (!cell.has_value())
        {
            stampFromCell.reset();

            return;
        }

        const auto a = antwika::voxel::cubeCornerOf(*stampFromCell);
        const auto b = antwika::voxel::cubeCornerOf(*cell);
        const auto lowX = std::min(a.x, b.x);
        const auto highX = std::max(a.x, b.x);
        const auto lowZ = std::min(a.z, b.z);
        const auto highZ = std::max(a.z, b.z);

        std::map<voxel::VoxelCell, voxel::VoxelCell> cubeCells;

        for (const auto voxel : map.voxels)
        {
            const auto corner =
                antwika::voxel::cubeCornerOf(voxel);

            if (corner.x < lowX || corner.x > highX
                || corner.z < lowZ || corner.z > highZ)
            {
                continue;
            }

            cubeCells.emplace(corner, voxel);
        }

        stampCells.clear();

        for (const auto &[corner, sample] : cubeCells)
        {
            stampCells.push_back(
                voxel::VoxelCell{
                    .x = corner.x - lowX,
                    .y = corner.y,
                    .z = corner.z - lowZ,
                    .kind = sample.kind,
                    .facing = sample.facing});
        }

        stampFromCell.reset();
    }

    std::vector<voxel::VoxelCell> Editor::stampGhost(
        const voxel::VoxelCell cell) const
    {
        std::vector<voxel::VoxelCell> cells;

        if (!stampCells.empty())
        {
            const auto corner = antwika::voxel::cubeCornerOf(cell);

            for (const auto offset : stampCells)
            {
                cells.push_back(
                    voxel::VoxelCell{
                        .x = corner.x + offset.x,
                        .y = offset.y,
                        .z = corner.z + offset.z});
            }

            return cells;
        }

        if (!stampFromCell.has_value())
        {
            return cells;
        }

        const auto a = antwika::voxel::cubeCornerOf(*stampFromCell);
        const auto b = antwika::voxel::cubeCornerOf(cell);

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
    } // GCOVR_EXCL_LINE

}
