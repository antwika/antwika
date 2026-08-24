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
            worldView.stamp.voxels.clear();
            worldView.stamp.fromPosition.reset();

            return;
        }

        if (!worldView.stamp.voxels.empty())
        {
            pushUndo();

            const auto stampCorner = antwika::voxel::cubeCornerOf(position);

            for (const auto &[offset, material] : worldView.stamp.voxels)
            {
                const voxel::VoxelPosition cornerPosition{
                    .x = stampCorner.x + offset.x,
                    .y = offset.y,
                    .z = stampCorner.z + offset.z};

                document.map.voxels = voxel::getWithRampsRebuilt(
                    voxel::withBlockAt(
                        document.map.voxels,
                        cornerPosition,
                        material.kind,
                        material.facing),
                    cornerPosition);
            }

            rebuildWorld();

            return;
        }

        worldView.stamp.fromPosition = position;
    }

    void Editor::finishStamp(const input::MouseButton button)
    {
        if (!worldView.stamp.fromPosition.has_value()
            || button != input::MouseButton::Left)
        {
            return;
        }

        const auto position = voxelmap::getCellUnder(
            getWorldCamera(play, cameraRig),
            getWorldRotation(play),
            camera::kCanvasSize,
            pointer.pointerOnCanvas,
            antwika::voxel::getCubeTop(worldView.worldEdit.editLevel));

        if (!position.has_value())
        {
            worldView.stamp.fromPosition.reset();

            return;
        }

        const auto a = antwika::voxel::cubeCornerOf(*worldView.stamp.fromPosition);
        const auto b = antwika::voxel::cubeCornerOf(*position);
        const auto lowX = std::min(a.x, b.x);
        const auto highX = std::max(a.x, b.x);
        const auto lowZ = std::min(a.z, b.z);
        const auto highZ = std::max(a.z, b.z);

        voxel::Voxels cubeVoxels;

        for (const auto &[position, material] : document.map.voxels)
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

        worldView.stamp.voxels.clear();

        for (const auto &[corner, sample] : cubeVoxels)
        {
            worldView.stamp.voxels[voxel::VoxelPosition{
                .x = corner.x - lowX,
                .y = corner.y,
                .z = corner.z - lowZ}] = sample;
        }

        worldView.stamp.fromPosition.reset();
    }

}
