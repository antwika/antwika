#include "antwika/editor/tools/ShapedCubes.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include <antwika/voxel/VoxelCube.hpp>

namespace antwika::editor
{

    std::vector<voxel::VoxelPosition> getShapedCubes(
        const voxel::VoxelPosition fromPosition,
        const voxel::VoxelPosition toPosition,
        const Paint paint)
    {
        const auto a = antwika::voxel::cubeCornerOf(fromPosition);
        const auto b = antwika::voxel::cubeCornerOf(toPosition);

        std::vector<voxel::VoxelPosition> positions;

        if (paint == Paint::Rect)
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

}
