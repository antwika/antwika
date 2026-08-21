#include "antwika/voxelmap/Voxel.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <set>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/SizeF.hpp>

#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/tilemap/TileEdges.hpp>

#include "VoxelDetail.hpp"

namespace antwika::voxelmap
{
    using namespace voxeldetail;

    namespace
    {

        [[nodiscard]] bool liesFlat(const Face &face)
        {
            return face.normal.y != 0.0F;
        }

        [[nodiscard]] gfx::Vec3 middleOf(
            const std::vector<voxel::VoxelCell> &cells)
        {
            if (cells.empty())
            {
                return gfx::Vec3{0.0F, 0.0F, 0.0F};
            }

            auto lowest = cells.front();
            auto highest = cells.front();

            for (const auto cell : cells)
            {
                lowest.x = std::min(lowest.x, cell.x);
                lowest.y = std::min(lowest.y, cell.y);
                lowest.z = std::min(lowest.z, cell.z);
                highest.x = std::max(highest.x, cell.x);
                highest.y = std::max(highest.y, cell.y);
                highest.z = std::max(highest.z, cell.z);
            }

            return gfx::Vec3{
                static_cast<float>(lowest.x + highest.x) / 2.0F,
                static_cast<float>(lowest.y + highest.y) / 2.0F,
                static_cast<float>(lowest.z + highest.z) / 2.0F};
        }
    }

    std::int32_t levelOf(const voxel::VoxelCell cell)
    {
        return cell.y;
    }

    std::int32_t topLevel(const std::vector<voxel::VoxelCell> &cells)
    {
        if (cells.empty())
        {
            return 0;
        }

        auto highest = levelOf(cells.front());

        for (const auto cell : cells)
        {
            highest = std::max(highest, levelOf(cell));
        }

        return highest;
    }

    std::int32_t bottomLevel(const std::vector<voxel::VoxelCell> &cells)
    {
        if (cells.empty())
        {
            return 0;
        }

        auto lowest = levelOf(cells.front());

        for (const auto cell : cells)
        {
            lowest = std::min(lowest, levelOf(cell));
        }

        return lowest;
    }

    std::vector<tilemap::Tile> defaultTiles(
        const std::vector<FaceRef> &faces)
    {
        std::vector<tilemap::Tile> tiles;

        tiles.reserve(faces.size());

        for (const auto face : faces)
        {
            tiles.push_back(
                tilemap::Tile{
                    .atlas = liesFlat(kVoxelFaces[face.side])
                           ? tilemap::Atlas::Floor
                           : tilemap::Atlas::Wall,
                    .index = static_cast<std::uint16_t>(
                        defaultTileIndex(face.cell, face.side))});
        }

        return tiles;
    } // GCOVR_EXCL_LINE

    gfx::Vec3 voxelsCenter(const std::vector<voxel::VoxelCell> &cells)
    {
        return middleOf(cells) * voxel::kVoxelSide;
    }

    gfx::Vec3 cellMiddle(const voxel::VoxelCell cell)
    {
        return gfx::Vec3{
            (static_cast<float>(cell.x) + 0.5F) * voxel::kVoxelSide,
            (static_cast<float>(cell.y) + 0.5F) * voxel::kVoxelSide,
            (static_cast<float>(cell.z) + 0.5F) * voxel::kVoxelSide};
    }

    std::size_t defaultTileIndex(
        const voxel::VoxelCell cell, const std::size_t face)
    {
        const auto apart = (cell.x * 3) + (cell.y * 5) + (cell.z * 7);
        const auto walkedIndex =
            (apart * static_cast<std::int32_t>(kFaces))
            + static_cast<std::int32_t>(face);
        const auto wrappedIndex =
            walkedIndex % static_cast<std::int32_t>(kTiles);

        return static_cast<std::size_t>(
            wrappedIndex < 0 ? wrappedIndex + static_cast<std::int32_t>(kTiles)
                       : wrappedIndex);
    }

    std::vector<voxel::VoxelCell> demoCells()
    {
        std::vector<voxel::VoxelCell> cells;

        cells.reserve(10);

        for (std::int32_t z = -1; z <= 1; ++z)
        {
            for (std::int32_t x = -1; x <= 1; ++x)
            {
                cells.push_back(voxel::VoxelCell{.x = x, .y = 0, .z = z});
            }
        }

        cells.push_back(voxel::VoxelCell{.x = 0, .y = 1, .z = 0});

        return cells;
    } // GCOVR_EXCL_LINE

    gfx::Mat4 modelRotation(
        const float yawRadians, const float pitchRadians)
    {
        const auto rotationMatrix = glm::rotate(
            gfx::identityMatrix(),
            pitchRadians,
            gfx::Vec3{1.0F, 0.0F, 0.0F});

        return glm::rotate(
            rotationMatrix, yawRadians, gfx::Vec3{0.0F, 1.0F, 0.0F});
    }

}
