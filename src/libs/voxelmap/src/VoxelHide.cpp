#include <algorithm>
#include <cmath>
#include <deque>
#include <map>
#include <utility>

#include <antwika/gfx/Bitmap.hpp>

#include <antwika/voxelmap/VoxelPick.hpp>

namespace antwika::voxelmap
{

    std::vector<LineSegment> occluderFootprints(
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

        const auto latticeAt = [](const std::int32_t x,
                           const std::int32_t y,
                           const std::int32_t z)
        {
            return cellMiddle(voxel::VoxelPosition{.x = x, .y = y, .z = z})
                   - gfx::Vec3{
                       voxel::kVoxelSide / 2.0F,
                       voxel::kVoxelSide / 2.0F,
                       voxel::kVoxelSide / 2.0F};
        };

        std::vector<LineSegment> segments;

        for (const auto &[column, level] : floors)
        {
            const auto &[x, z] = column;
            const auto corner = latticeAt(x, level, z);
            const auto acrossPoint = latticeAt(x + 1, level, z);
            const auto alongPoint = latticeAt(x, level, z + 1);
            const auto both = latticeAt(x + 1, level, z + 1);

            segments.push_back(
                LineSegment{
                    .fromPosition = corner,
                    .toPosition = acrossPoint});
            segments.push_back(
                LineSegment{
                    .fromPosition = corner,
                    .toPosition = alongPoint});
            segments.push_back(
                LineSegment{
                    .fromPosition = acrossPoint,
                    .toPosition = both});
            segments.push_back(
                LineSegment{
                    .fromPosition = alongPoint,
                    .toPosition = both});
        }

        return segments;
    } // GCOVR_EXCL_LINE

    voxel::VoxelPosition occlusionMaskOrigin(
        const voxel::VoxelPosition aboutPosition)
    {
        const auto arm =
            static_cast<std::int32_t>(kOcclusionMaskWidth) / 2;

        return voxel::VoxelPosition{
            .x = aboutPosition.x - arm, .z = aboutPosition.z - arm};
    }

    gfx::Bitmap occlusionMask(
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
