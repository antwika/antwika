#pragma once

#include <compare>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include <antwika/gfx/Math3D.hpp>
#include <antwika/gfx/MeshData.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/gfx/Size.hpp>

#include <antwika/tilemap/Tilemap.hpp>
#include <antwika/voxel/VoxelCell.hpp>
#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/voxel/Voxels.hpp>
#include <antwika/voxelmap/VoxelStairs.hpp>

namespace antwika::voxelmap
{

    inline constexpr std::uint8_t kWaterAlpha = 255;

    enum class Pass : std::uint8_t
    {
        Solid,
        Water,
    };

    inline constexpr std::size_t kVoxelFaceCount = 6;

    [[nodiscard]] gfx::Vec3 getFaceNormal(std::size_t side);

    [[nodiscard]] voxel::StairPart stairPartOf(
        voxel::VoxelPosition climbPosition, std::size_t side);

    [[nodiscard]] gfx::Vec3 getVoxelsCenter(const voxel::Voxels &voxels);

    [[nodiscard]] gfx::Vec3 getCellMiddle(voxel::VoxelPosition position);

    [[nodiscard]] gfx::Vec3 getFaceCorner(
        std::size_t side, std::size_t corner);

    [[nodiscard]] std::size_t getDefaultTileIndex(
        voxel::VoxelPosition position, std::size_t face);

    struct FaceRef final
    {
        voxel::VoxelCell cell{};

        std::size_t side = 0;

        voxel::VoxelPosition climbPosition{};

        voxel::StairHalf levelHalf = voxel::StairHalf::Any;

        [[nodiscard]] bool operator==(const FaceRef &other) const
        {
            return cell.position == other.cell.position
                   && side == other.side;
        }

        [[nodiscard]] std::strong_ordering operator<=>(
            const FaceRef &other) const
        {
            if (const auto where = cell.position <=> other.cell.position;
                where != 0)
            {
                return where;
            }

            return side <=> other.side;
        }
    };

    [[nodiscard]] bool usesMirroredUv(
        const voxel::Voxels &voxels, const FaceRef &face);

    [[nodiscard]] std::vector<FaceRef> visibleFacesOf(
        const voxel::Voxels &voxels);

    [[nodiscard]] std::int32_t levelOf(voxel::VoxelPosition position);

    [[nodiscard]] std::int32_t getTopLevel(const voxel::Voxels &voxels);

    [[nodiscard]] std::int32_t getBottomLevel(const voxel::Voxels &voxels);

    [[nodiscard]] std::vector<tilemap::Tile> getDefaultTiles(
        const std::vector<FaceRef> &faces);

    [[nodiscard]] voxel::Voxels getDemoCells();

    [[nodiscard]] gfx::MeshData getVoxelMesh(const voxel::Voxels &voxels);

    [[nodiscard]] gfx::MeshData getVoxelMesh(
        const voxel::Voxels &voxels,
        std::span<const tilemap::Tile> wovenTiles,
        Pass pass = Pass::Solid);

    [[nodiscard]] gfx::MeshData getVoxelMesh(
        const voxel::Voxels &voxels,
        std::span<const FaceRef> faces,
        std::span<const tilemap::Tile> wovenTiles,
        Pass pass);

    inline constexpr std::size_t kMeshPieceVertices = 60000;

    inline constexpr std::int32_t kMeshRegionSide = 16;

    [[nodiscard]] voxel::VoxelPosition getMeshRegionOf(
        voxel::VoxelPosition position, std::int32_t regionSide);

    [[nodiscard]] std::vector<gfx::MeshData> getVoxelMeshPieces(
        const voxel::Voxels &voxels,
        std::span<const FaceRef> faces,
        std::span<const tilemap::Tile> wovenTiles,
        Pass pass,
        std::int32_t regionSide = kMeshRegionSide);

    [[nodiscard]] gfx::Mat4 getModelRotation(
        float yawRadians, float pitchRadians);

}
