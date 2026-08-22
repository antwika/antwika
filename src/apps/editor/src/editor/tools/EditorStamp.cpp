#include <antwika/input/MouseButton.hpp>
#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/voxelmap/VoxelPick.hpp>

#include "antwika/editor/Editor.hpp"

namespace antwika::editor
{

    void Editor::pressStamp(
        const voxel::VoxelPosition position, const input::MouseButton button)
    {
        if (button == input::MouseButton::Right)
        {
            stampVoxels.clear();
            stampFromPosition.reset();

            return;
        }

        if (!stampVoxels.empty())
        {
            pushUndo();

            const auto stampCorner = antwika::voxel::cubeCornerOf(position);

            for (const auto &[offset, material] : stampVoxels)
            {
                const voxel::VoxelPosition cornerPosition{
                    .x = stampCorner.x + offset.x,
                    .y = offset.y,
                    .z = stampCorner.z + offset.z};

                map.voxels = voxel::withRampsRebuilt(
                    voxel::withBlockAt(
                        map.voxels,
                        cornerPosition,
                        material.kind,
                        material.facing),
                    cornerPosition);
            }

            rebuildWorld();

            return;
        }

        stampFromPosition = position;
    }

    void Editor::finishStamp(const input::MouseButton button)
    {
        if (!stampFromPosition.has_value()
            || button != input::MouseButton::Left)
        {
            return;
        }

        const auto position = voxelmap::cellUnder(
            worldCamera(),
            worldRotation(),
            camera::kCanvasSize,
            pointer.pointerOnCanvas,
            antwika::voxel::cubeTop(editLevel));

        if (!position.has_value())
        {
            stampFromPosition.reset();

            return;
        }

        const auto a = antwika::voxel::cubeCornerOf(*stampFromPosition);
        const auto b = antwika::voxel::cubeCornerOf(*position);
        const auto lowX = std::min(a.x, b.x);
        const auto highX = std::max(a.x, b.x);
        const auto lowZ = std::min(a.z, b.z);
        const auto highZ = std::max(a.z, b.z);

        voxel::Voxels cubeVoxels;

        for (const auto &[position, material] : map.voxels)
        {
            const auto corner =
                antwika::voxel::cubeCornerOf(position);

            if (corner.x < lowX || corner.x > highX
                || corner.z < lowZ || corner.z > highZ)
            {
                continue;
            }

            cubeVoxels.emplace(corner, material);
        }

        stampVoxels.clear();

        for (const auto &[corner, sample] : cubeVoxels)
        {
            stampVoxels[voxel::VoxelPosition{
                .x = corner.x - lowX,
                .y = corner.y,
                .z = corner.z - lowZ}] = sample;
        }

        stampFromPosition.reset();
    }

    std::vector<voxel::VoxelPosition> Editor::stampGhost(
        const voxel::VoxelPosition position) const
    {
        std::vector<voxel::VoxelPosition> positions;

        if (!stampVoxels.empty())
        {
            const auto corner = antwika::voxel::cubeCornerOf(position);

            for (const auto &[offset, material] : stampVoxels)
            {
                positions.push_back(
                    voxel::VoxelPosition{
                        .x = corner.x + offset.x,
                        .y = offset.y,
                        .z = corner.z + offset.z});
            }

            return positions;
        }

        if (!stampFromPosition.has_value())
        {
            return positions;
        }

        const auto a = antwika::voxel::cubeCornerOf(*stampFromPosition);
        const auto b = antwika::voxel::cubeCornerOf(position);

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
    } // GCOVR_EXCL_LINE

}
