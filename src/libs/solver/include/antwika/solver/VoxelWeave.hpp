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

    [[nodiscard]] std::size_t tileToIndex(tilemap::Tile tile);

    [[nodiscard]] tilemap::Tile tileFromIndex(std::size_t value);

    [[nodiscard]] std::vector<FaceSeam> faceAdjacency(
        const std::vector<voxelmap::FaceRef> &faces,
        CornerSeams corners = CornerSeams::Included);

    [[nodiscard]] std::vector<FaceSeam> satisfiedSeams(
        const std::vector<voxelmap::FaceRef> &faces,
        std::span<const tilemap::Tile> drawnTiles,
        const tile::TileRules &rules,
        CornerSeams corners = CornerSeams::Included);

    [[nodiscard]] bool isCornerSeam(
        const std::vector<voxelmap::FaceRef> &faces, const FaceSeam &seam);

    [[nodiscard]] std::vector<FaceSeam> sameLevelSeams(
        const std::vector<voxelmap::FaceRef> &faces,
        const std::vector<FaceSeam> &seams,
        std::int32_t level);

    [[nodiscard]] std::vector<FaceSeam> crossLevelSeams(
        const std::vector<voxelmap::FaceRef> &faces,
        const std::vector<FaceSeam> &seams,
        std::int32_t level);

    [[nodiscard]] std::optional<std::vector<tilemap::Tile>> solvedTiles(
        const std::vector<voxelmap::FaceRef> &faces,
        const tile::TileRules &rules,
        CornerSeams corners = CornerSeams::Included);

    [[nodiscard]] TileSolve solveTiles(
        const std::vector<voxelmap::FaceRef> &faces,
        const tile::TileRules &rules,
        CornerSeams corners = CornerSeams::Included);

    [[nodiscard]] std::vector<WeaveGap> missingRules(
        const std::vector<voxelmap::FaceRef> &faces,
        const tile::TileRules &rules,
        CornerSeams corners = CornerSeams::Included);

    [[nodiscard]] std::string weaveErrorMessage(
        const std::vector<voxelmap::FaceRef> &faces,
        const tile::TileRules &rules,
        const TileSolve &solve,
        CornerSeams corners = CornerSeams::Included);

}
