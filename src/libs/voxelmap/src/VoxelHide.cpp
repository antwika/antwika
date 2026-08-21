#include <algorithm>
#include <cmath>
#include <deque>
#include <map>
#include <set>
#include <utility>

#include <antwika/gfx/Bitmap.hpp>

#include <antwika/voxelmap/VoxelPick.hpp>

namespace antwika::voxelmap
{

    std::vector<LineSegment> occluderFootprints(
        const std::set<voxel::VoxelCell> &hiddenCells)
    {
        std::map<std::pair<std::int32_t, std::int32_t>, std::int32_t>
            floors;

        for (const auto cell : hiddenCells)
        {
            const auto key = std::pair{cell.x, cell.z};
            const auto foundFloor = floors.find(key);

            if (foundFloor == floors.end() || cell.y < foundFloor->second)
            {
                floors[key] = cell.y;
            }
        }

        const auto latticeAt = [](const std::int32_t x,
                           const std::int32_t y,
                           const std::int32_t z)
        {
            return cellMiddle(voxel::VoxelCell{.x = x, .y = y, .z = z})
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
            const auto acrossCell = latticeAt(x + 1, level, z);
            const auto alongCell = latticeAt(x, level, z + 1);
            const auto both = latticeAt(x + 1, level, z + 1);

            segments.push_back(
                LineSegment{
                    .fromPosition = corner,
                    .toPosition = acrossCell});
            segments.push_back(
                LineSegment{
                    .fromPosition = corner,
                    .toPosition = alongCell});
            segments.push_back(
                LineSegment{
                    .fromPosition = acrossCell,
                    .toPosition = both});
            segments.push_back(
                LineSegment{
                    .fromPosition = alongCell,
                    .toPosition = both});
        }

        return segments;
    } // GCOVR_EXCL_LINE

    voxel::VoxelCell occlusionMaskOrigin(const voxel::VoxelCell aboutCell)
    {
        const auto arm =
            static_cast<std::int32_t>(kOcclusionMaskWidth) / 2;

        return voxel::VoxelCell{.x = aboutCell.x - arm, .z = aboutCell.z - arm};
    }

    gfx::Bitmap occlusionMask(
        const std::set<voxel::VoxelCell> &hiddenCells,
        const voxel::VoxelCell cornerCell)
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

        for (const auto cell : hiddenCells)
        {
            const auto acrossOffset = cell.x - cornerCell.x;
            const auto alongOffset = cell.z - cornerCell.z;
            const auto level = cell.y;

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
