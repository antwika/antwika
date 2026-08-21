#pragma once

#include <cstddef>
#include <cstdint>
#include <set>
#include <span>
#include <vector>

#include <antwika/gfx/Math3D.hpp>
#include <antwika/gfx/MeshData.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/gfx/Size.hpp>

#include <antwika/tilemap/Tilemap.hpp>
#include <antwika/voxel/VoxelCell.hpp>
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

    [[nodiscard]] gfx::Vec3 faceNormal(std::size_t side);

    [[nodiscard]] voxel::StairPart stairPartOf(
        voxel::VoxelCell climbCell, std::size_t side);

    [[nodiscard]] gfx::Vec3 voxelsCenter(
        const std::vector<voxel::VoxelCell> &cells);

    [[nodiscard]] gfx::Vec3 cellMiddle(voxel::VoxelCell cell);

    [[nodiscard]] gfx::Vec3 faceCorner(
        std::size_t side, std::size_t corner);

    [[nodiscard]] std::size_t defaultTileIndex(
        voxel::VoxelCell cell, std::size_t face);

    struct FaceRef final
    {
        voxel::VoxelCell cell{};

        std::size_t side = 0;

        voxel::VoxelCell climbCell{};

        voxel::StairHalf levelHalf = voxel::StairHalf::Any;

        [[nodiscard]] bool operator==(const FaceRef &other) const
        {
            return cell == other.cell && side == other.side;
        }

        [[nodiscard]] std::strong_ordering operator<=>(
            const FaceRef &other) const
        {
            if (const auto where = cell <=> other.cell; where != 0)
            {
                return where;
            }

            return side <=> other.side;
        }
    };

    [[nodiscard]] bool usesMirroredUv(
        const std::vector<voxel::VoxelCell> &cells, const FaceRef &face);

    [[nodiscard]] std::vector<FaceRef> visibleFacesOf(
        const std::vector<voxel::VoxelCell> &cells);

    [[nodiscard]] std::int32_t levelOf(voxel::VoxelCell cell);

    [[nodiscard]] std::int32_t topLevel(
        const std::vector<voxel::VoxelCell> &cells);

    [[nodiscard]] std::int32_t bottomLevel(
        const std::vector<voxel::VoxelCell> &cells);

    [[nodiscard]] std::vector<tilemap::Tile> defaultTiles(
        const std::vector<FaceRef> &faces);

    [[nodiscard]] std::vector<voxel::VoxelCell> demoCells();

    [[nodiscard]] gfx::MeshData voxelMesh(
        const std::vector<voxel::VoxelCell> &cells);

    [[nodiscard]] gfx::MeshData voxelMesh(
        const std::vector<voxel::VoxelCell> &cells,
        std::span<const tilemap::Tile> wovenTiles,
        Pass pass = Pass::Solid);

    [[nodiscard]] gfx::MeshData voxelMesh(
        const std::vector<voxel::VoxelCell> &cells,
        std::span<const FaceRef> faces,
        std::span<const tilemap::Tile> wovenTiles,
        Pass pass);

    inline constexpr std::size_t kMeshPieceVertices = 60000;

    [[nodiscard]] gfx::Mat4 modelRotation(
        float yawRadians, float pitchRadians);

}
