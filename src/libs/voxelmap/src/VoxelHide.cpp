#include <algorithm>
#include <cmath>
#include <deque>
#include <map>
#include <utility>

#include <antwika/gfx/Bitmap.hpp>

#include <antwika/voxelmap/VoxelPick.hpp>

namespace antwika::voxelmap
{

    std::vector<LineSegment> getOccluderFootprints(
        const voxel::Voxels &hiddenVoxels)
    {
        std::map<std::pair<std::int32_t, std::int32_t>, std::int32_t>
            floors;

        for (const auto &[position, material] : hiddenVoxels)
        {
            const auto key = std::pair{position.x, position.z};
            const auto foundFloor = floors.find(key);

            if (foundFloor == floors.end() || position.y < foundFloor->second)
            {
                floors[key] = position.y;
            }
        }

        std::vector<LineSegment> segments;

        for (const auto &[column, level] : floors)
        {
            const auto &[x, z] = column;
            const auto rimSegments = getCellRimSegments(
                CellRim{.cellX = x, .cellZ = z, .latticeFoot = level});

            segments.insert(
                segments.end(), rimSegments.begin(), rimSegments.end());
        }

        return segments;
    } // GCOVR_EXCL_LINE

    static_assert(
        kOcclusionMaskLevels <= 8 * gfx::kBytesPerPixel,
        "every occlusion mask level must fit the bits of one pixel");

    voxel::VoxelPosition getOcclusionMaskOrigin(
        const voxel::VoxelPosition aboutPosition)
    {
        const auto arm =
            static_cast<std::int32_t>(kOcclusionMaskWidth) / 2;

        return voxel::VoxelPosition{
            .x = aboutPosition.x - arm, .z = aboutPosition.z - arm};
    }

    gfx::Bitmap getOcclusionMask(
        const voxel::Voxels &hiddenVoxels,
        const voxel::VoxelPosition cornerPosition)
    {
        gfx::Bitmap bitmap{
            .size =
                gfx::Size{
                    .width = kOcclusionMaskWidth,
                    .height = kOcclusionMaskWidth},
            .pixels = std::vector<std::uint8_t>(
                static_cast<std::size_t>(kOcclusionMaskWidth)
                    * static_cast<std::size_t>(kOcclusionMaskWidth)
                    * gfx::kBytesPerPixel,
                0)};

        for (const auto &[position, material] : hiddenVoxels)
        {
            const auto acrossOffset = position.x - cornerPosition.x;
            const auto alongOffset = position.z - cornerPosition.z;
            const auto level = position.y;

            if (acrossOffset < 0 || alongOffset < 0
                || acrossOffset >= static_cast<std::int32_t>(
                       kOcclusionMaskWidth)
                || alongOffset >= static_cast<std::int32_t>(
                       kOcclusionMaskWidth)
                || level < 0
                || level >= static_cast<std::int32_t>(
                       kOcclusionMaskLevels))
            {
                continue;
            }

            const auto cellIndex =
                ((static_cast<std::size_t>(alongOffset)
                  * kOcclusionMaskWidth)
                 + static_cast<std::size_t>(acrossOffset))
                * gfx::kBytesPerPixel;

            bitmap.pixels[cellIndex + (static_cast<std::size_t>(level) / 8)] |=
                static_cast<std::uint8_t>(1U << (level % 8));
        }

        return bitmap;
    } // GCOVR_EXCL_LINE

}
