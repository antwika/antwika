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

        [[nodiscard]] gfx::Vec3 middleOf(const voxel::Voxels &voxels)
        {
            if (voxels.empty())
            {
                return gfx::Vec3{0.0F, 0.0F, 0.0F};
            }

            auto lowest = voxels.begin()->first;
            auto highest = voxels.begin()->first;

            for (const auto &[position, material] : voxels)
            {
                lowest.x = std::min(lowest.x, position.x);
                lowest.y = std::min(lowest.y, position.y);
                lowest.z = std::min(lowest.z, position.z);
                highest.x = std::max(highest.x, position.x);
                highest.y = std::max(highest.y, position.y);
                highest.z = std::max(highest.z, position.z);
            }

            return gfx::Vec3{
                static_cast<float>(lowest.x + highest.x) / 2.0F,
                static_cast<float>(lowest.y + highest.y) / 2.0F,
                static_cast<float>(lowest.z + highest.z) / 2.0F};
        }
    }

    std::int32_t levelOf(const voxel::VoxelPosition position)
    {
        return position.y;
    }

    std::int32_t getTopLevel(const voxel::Voxels &voxels)
    {
        if (voxels.empty())
        {
            return 0;
        }

        auto highest = levelOf(voxels.begin()->first);

        for (const auto &[position, material] : voxels)
        {
            highest = std::max(highest, levelOf(position));
        }

        return highest;
    }

    std::int32_t getBottomLevel(const voxel::Voxels &voxels)
    {
        if (voxels.empty())
        {
            return 0;
        }

        auto lowest = levelOf(voxels.begin()->first);

        for (const auto &[position, material] : voxels)
        {
            lowest = std::min(lowest, levelOf(position));
        }

        return lowest;
    }

    std::vector<tilemap::Tile> getDefaultTiles(
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
                        getDefaultTileIndex(face.cell.position, face.side))});
        }

        return tiles;
    } // GCOVR_EXCL_LINE

    gfx::Vec3 getVoxelsCenter(const voxel::Voxels &voxels)
    {
        return middleOf(voxels) * voxel::kVoxelSide;
    }

    gfx::Vec3 getCellMiddle(const voxel::VoxelPosition position)
    {
        return gfx::Vec3{
            (static_cast<float>(position.x) + 0.5F) * voxel::kVoxelSide,
            (static_cast<float>(position.y) + 0.5F) * voxel::kVoxelSide,
            (static_cast<float>(position.z) + 0.5F) * voxel::kVoxelSide};
    }

    std::size_t getDefaultTileIndex(
        const voxel::VoxelPosition position, const std::size_t face)
    {
        const auto apart =
            (position.x * 3) + (position.y * 5) + (position.z * 7);
        const auto walkedIndex =
            (apart * static_cast<std::int32_t>(kFaces))
            + static_cast<std::int32_t>(face);
        const auto wrappedIndex =
            walkedIndex % static_cast<std::int32_t>(kTiles);

        return static_cast<std::size_t>(
            wrappedIndex < 0 ? wrappedIndex + static_cast<std::int32_t>(kTiles)
                       : wrappedIndex);
    }

    voxel::Voxels getDemoCells()
    {
        voxel::Voxels voxels;

        for (std::int32_t z = -1; z <= 1; ++z)
        {
            for (std::int32_t x = -1; x <= 1; ++x)
            {
                voxels[voxel::VoxelPosition{.x = x, .y = 0, .z = z}] =
                    voxel::VoxelMaterial{};
            }
        }

        voxels[voxel::VoxelPosition{.x = 0, .y = 1, .z = 0}] =
            voxel::VoxelMaterial{};

        return voxels;
    } // GCOVR_EXCL_LINE

    gfx::Mat4 getModelRotation(
        const float yawRadians, const float pitchRadians)
    {
        const auto rotationMatrix = glm::rotate(
            gfx::getIdentityMatrix(),
            pitchRadians,
            gfx::Vec3{1.0F, 0.0F, 0.0F});

        return glm::rotate(
            rotationMatrix, yawRadians, gfx::Vec3{0.0F, 1.0F, 0.0F});
    }

}
