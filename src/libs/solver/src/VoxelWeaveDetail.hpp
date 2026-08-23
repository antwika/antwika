#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <limits>
#include <map>
#include <set>
#include <vector>

#include <antwika/gfx/Math3D.hpp>
#include <antwika/voxelmap/Voxel.hpp>
#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/tilemap/TileEdges.hpp>

#include <antwika/tile/TileRules.hpp>

namespace antwika::solver::weavedetail
{

    constexpr std::size_t kNoFaceIndex =
        std::numeric_limits<std::size_t>::max();

    constexpr std::size_t kAxisCount = 3;

    constexpr std::size_t kTopLeftCorner = 3;

    constexpr std::size_t kTopRightCorner = 2;

    constexpr std::size_t kBottomLeftCorner = 0;

    constexpr std::uint64_t kMaxSteps = 2'000'000;

    constexpr std::size_t kMaxReportedConflicts = 6;

    using DomainKey = std::pair<tilemap::Atlas, voxel::Kind>;

    [[nodiscard]] std::map<DomainKey, std::set<tilemap::Tile>>
    getRuledTilesByDomain(
        const tile::TileRules &rules);

    [[nodiscard]] std::size_t faceAlong(const gfx::Vec3 direction);

    [[nodiscard]] bool isAtCubeFace(
        voxel::VoxelPosition position, gfx::Vec3 direction);

    struct FaceEdge final
    {
        tilemap::TileEdge edge{};

        bool atRim = false;
    };

        [[nodiscard]] voxel::VoxelPosition getOffsetBy(
            voxel::VoxelPosition fromPosition, gfx::Vec3 offset);

        [[nodiscard]] gfx::Vec3 acrossOf(const std::size_t side);

        [[nodiscard]] gfx::Vec3 downOf(const std::size_t side);

        [[nodiscard]] tilemap::Atlas atlasOf(const std::size_t side);

        [[nodiscard]] std::set<tilemap::Tile> tilesFor(
            const tile::TileRules &rules,
            const std::set<tilemap::Tile> &spokenTiles,
            const voxelmap::FaceRef &face);

        [[nodiscard]] bool edgesCompatible(
            const tile::TileRules &rules,
            const tilemap::Tile hereTile,
            const tilemap::TileEdge hereEdge,
            const tilemap::Tile thereTile,
            const tilemap::TileEdge thereEdge);

        [[nodiscard]] voxel::Side getSideTowards(
            const std::size_t face, const gfx::Vec3 direction);

        [[nodiscard]] bool isSameSurface(
            const voxelmap::FaceRef &oneFace,
            const voxelmap::FaceRef &otherFace);

        [[nodiscard]] bool isSameSurface(
            const std::map<voxelmap::FaceRef, std::size_t> &faceIndexes,
            const std::vector<voxelmap::FaceRef> &faces,
            const voxelmap::FaceRef placeFace,
            const voxelmap::FaceRef &face);

        [[nodiscard]] std::vector<FaceEdge> edgesOf(
            const std::map<voxelmap::FaceRef, std::size_t> &faceIndexes,
            const std::vector<voxelmap::FaceRef> &faces,
            const voxelmap::FaceRef face);

        [[nodiscard]] std::map<voxel::Corner, bool> getCornersBeyond(
            const std::map<voxelmap::FaceRef, std::size_t> &faceIndexes,
            const std::vector<voxelmap::FaceRef> &faces,
            const voxelmap::FaceRef face);

        [[nodiscard]] std::map<voxelmap::FaceRef, std::size_t> getFacesByPlace(
            const std::vector<voxelmap::FaceRef> &faces);

}
