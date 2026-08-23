#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include <antwika/tilemap/AtlasLayout.hpp>
#include <antwika/tilemap/Tilemap.hpp>
#include <antwika/voxelmap/Voxel.hpp>
#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/tilemap/TileEdges.hpp>

#include <antwika/tile/TileRules.hpp>

#include "antwika/solver/FaceSeam.hpp"
#include "antwika/solver/SolveFailure.hpp"
#include "antwika/solver/TileSolve.hpp"
#include "antwika/solver/WeaveGap.hpp"

namespace antwika::solver
{

    inline constexpr std::size_t kAtlasTiles =
        static_cast<std::size_t>(tilemap::kAtlasColumns * tilemap::kAtlasRows);

    inline constexpr std::size_t kTileDomainSize = kAtlasTiles * 2;

    enum class CornerSeams : std::uint8_t
    {
        Ignored,
        Included,
    };

    [[nodiscard]] std::size_t getTileToIndex(tilemap::Tile tile);

    [[nodiscard]] tilemap::Tile getTileFromIndex(std::size_t value);

    [[nodiscard]] std::vector<FaceSeam> getFaceAdjacency(
        const std::vector<voxelmap::FaceRef> &faces,
        CornerSeams corners = CornerSeams::Included);

    [[nodiscard]] std::vector<FaceSeam> getSatisfiedSeams(
        const std::vector<voxelmap::FaceRef> &faces,
        std::span<const tilemap::Tile> drawnTiles,
        const tile::TileRules &rules,
        CornerSeams corners = CornerSeams::Included);

    [[nodiscard]] bool isCornerSeam(
        const std::vector<voxelmap::FaceRef> &faces, const FaceSeam &seam);

    [[nodiscard]] std::vector<FaceSeam> getSameLevelSeams(
        const std::vector<voxelmap::FaceRef> &faces,
        const std::vector<FaceSeam> &seams,
        std::int32_t level);

    [[nodiscard]] std::vector<FaceSeam> getCrossLevelSeams(
        const std::vector<voxelmap::FaceRef> &faces,
        const std::vector<FaceSeam> &seams,
        std::int32_t level);

    [[nodiscard]] std::optional<std::vector<tilemap::Tile>> getSolvedTiles(
        const std::vector<voxelmap::FaceRef> &faces,
        const tile::TileRules &rules,
        CornerSeams corners = CornerSeams::Included);

    [[nodiscard]] TileSolve getSolveTiles(
        const std::vector<voxelmap::FaceRef> &faces,
        const tile::TileRules &rules,
        CornerSeams corners = CornerSeams::Included);

    [[nodiscard]] std::vector<WeaveGap> getMissingRules(
        const std::vector<voxelmap::FaceRef> &faces,
        const tile::TileRules &rules,
        CornerSeams corners = CornerSeams::Included);

    [[nodiscard]] std::string getWeaveErrorMessage(
        const std::vector<voxelmap::FaceRef> &faces,
        const tile::TileRules &rules,
        const TileSolve &solve,
        CornerSeams corners = CornerSeams::Included);

}
