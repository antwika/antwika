#pragma once

#include <vector>

#include <antwika/voxelmap/Voxel.hpp>
#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/tilemap/TileEdges.hpp>

#include <antwika/tile/TileRules.hpp>

namespace antwika::tile
{

    [[nodiscard]] bool hasBorder(
        const TileRules &rules, tilemap::Tile tile, tilemap::TileEdge edge);

    [[nodiscard]] bool shapesCompatible(
        const TileRules &rules,
        tilemap::Tile tile,
        tilemap::TileEdge edge,
        tilemap::Tile otherTile);

    struct ShapedJunctions final
    {
        std::vector<TileRule> toAddRules{};

        std::vector<TileRule> conflictingRules{};

        [[nodiscard]] bool operator==(
            const ShapedJunctions &other) const = default;
    };

    [[nodiscard]] ShapedJunctions rulesFromShapes(
        const TileRules &rules, voxel::Kind kind);

}
