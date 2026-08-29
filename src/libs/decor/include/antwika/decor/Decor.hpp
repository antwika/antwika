#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include <antwika/time/Tick.hpp>
#include <antwika/tilemap/Tilemap.hpp>
#include <antwika/voxelmap/Voxel.hpp>

#include <antwika/tile/TileRules.hpp>

namespace antwika::decor
{

    inline constexpr std::size_t kMaxDecorFrames = 8;

    inline constexpr time::Tick kDecorPaceTick = 24;

    inline constexpr std::uint8_t kFullFrequency = 100;

    inline constexpr std::uint8_t kMaxDecorSpan = 4;

    struct DecorTile final
    {
        tilemap::Tile tile{};

        std::vector<tilemap::Tile> frameTiles{};

        std::vector<tilemap::Tile> allowedBaseTiles{};

        std::uint8_t frequency = kFullFrequency;

        std::uint8_t weight = kFullFrequency;

        std::size_t layer = 1;

        std::uint8_t width = 1;

        std::uint8_t height = 1;

        std::vector<tilemap::Tile> spanTiles{};

        [[nodiscard]] bool operator==(const DecorTile &other) const
            = default;
    };

    [[nodiscard]] const DecorTile *decorOf(
        std::span<const DecorTile> decor, tilemap::Tile tile);

    [[nodiscard]] std::vector<DecorTile> getWithDecorToggled(
        const std::vector<DecorTile> &decor,
        tilemap::Tile tile,
        std::size_t layer = 1);

    [[nodiscard]] std::vector<DecorTile> getWithBaseToggled(
        const std::vector<DecorTile> &decor,
        tilemap::Tile tile,
        tilemap::Tile baseTile);

    [[nodiscard]] std::vector<DecorTile> getWithFrameAdded(
        const std::vector<DecorTile> &decor, tilemap::Tile tile);

    [[nodiscard]] bool isDecorSpanned(const DecorTile &decor);

    [[nodiscard]] std::vector<DecorTile> getWithSpanSet(
        const std::vector<DecorTile> &decor,
        tilemap::Tile tile,
        std::uint8_t acrossSpan,
        std::uint8_t downSpan);

    [[nodiscard]] std::vector<DecorTile> getWithMemberSet(
        const std::vector<DecorTile> &decor,
        tilemap::Tile tile,
        std::size_t member,
        tilemap::Tile drawnTile);

    [[nodiscard]] std::vector<DecorTile> getWithDecorLayerSet(
        const std::vector<DecorTile> &decor,
        tilemap::Tile tile,
        std::size_t layer);

    [[nodiscard]] std::vector<DecorTile> getWithWeightSet(
        const std::vector<DecorTile> &decor,
        tilemap::Tile tile,
        std::uint8_t weight);

    [[nodiscard]] std::vector<DecorTile> getWithFrequencySet(
        const std::vector<DecorTile> &decor,
        tilemap::Tile tile,
        std::uint8_t frequency);

    [[nodiscard]] std::vector<DecorTile> getWithFrameSet(
        const std::vector<DecorTile> &decor,
        tilemap::Tile tile,
        std::size_t frame,
        tilemap::Tile drawnTile);

    [[nodiscard]] std::vector<DecorTile> getCompactedDecor(
        const std::vector<DecorTile> &decor);

    [[nodiscard]] bool tilesCompatible(
        const tile::TileRules &rules,
        tilemap::Tile tile,
        tilemap::TileEdge edge,
        tilemap::Tile otherTile);

    [[nodiscard]] std::optional<std::vector<std::optional<tilemap::Tile>>>
    getPreviewNeighbourhood(
        const tile::TileRules &rules,
        tilemap::Tile middleTile,
        std::size_t side,
        std::uint32_t seed);

    [[nodiscard]] std::map<std::size_t, tilemap::Tile> getSolveDecor(
        const std::vector<voxelmap::FaceRef> &faces,
        std::span<const tilemap::Tile> drawnTiles,
        std::span<const DecorTile> decor,
        const tile::TileRules &decorRules,
        std::uint32_t seed);

    [[nodiscard]] std::vector<
        std::pair<std::size_t, std::map<std::size_t, tilemap::Tile>>>
    getSolveDecorLayers(
        const std::vector<voxelmap::FaceRef> &faces,
        std::span<const tilemap::Tile> drawnTiles,
        std::span<const DecorTile> decor,
        const tile::TileRules &decorRules,
        std::uint32_t seed);

    [[nodiscard]] tilemap::Tile decorFrameAt(
        const DecorTile &decor, time::Tick tick);

    inline constexpr float kDecorDepthBias = 0.01F;

    [[nodiscard]] gfx::MeshData getDecorMesh(
        const std::vector<voxelmap::FaceRef> &faces,
        const std::map<std::size_t, tilemap::Tile> &placedTiles,
        std::span<const DecorTile> decor,
        time::Tick tick,
        float lift = kDecorDepthBias);

    [[nodiscard]] bool hasAnimatedDecor(std::span<const DecorTile> decor);

}
