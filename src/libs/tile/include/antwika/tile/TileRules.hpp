#pragma once

#include <cstddef>
#include <map>
#include <set>
#include <optional>
#include <utility>
#include <vector>

#include <antwika/tilemap/Tilemap.hpp>
#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/voxel/VoxelStairs.hpp>
#include <antwika/tilemap/TileEdges.hpp>

#include "antwika/tile/TileRule.hpp"

namespace antwika::tile
{

    class TileRules final
    {
    public:
        [[nodiscard]] bool allows(
            tilemap::Tile tile,
            tilemap::TileEdge edge,
            tilemap::Tile neighbourTile) const;

        void toggle(
            tilemap::Tile tile,
            tilemap::TileEdge edge,
            tilemap::Tile neighbourTile);

        void setAllows(
            tilemap::Tile tile,
            tilemap::TileEdge edge,
            tilemap::Tile neighbourTile,
            bool allowed);

        void allow(
            tilemap::Tile tile,
            tilemap::TileEdge edge,
            tilemap::Tile neighbourTile);

        [[nodiscard]] bool hasNoRule(
            tilemap::Tile tile,
            tilemap::TileEdge edge) const;

        [[nodiscard]] bool isForbidden(
            tilemap::Tile tile,
            tilemap::TileEdge edge) const;

        [[nodiscard]] bool allowsBoundary(
            tilemap::Tile tile, tilemap::TileEdge edge) const;

        [[nodiscard]] bool boundaryOnly(
            tilemap::Tile tile, tilemap::TileEdge edge) const;

        void setAllowsBoundary(
            tilemap::Tile tile,
            tilemap::TileEdge edge,
            bool may);

        void forbidAll(tilemap::Tile tile, tilemap::TileEdge edge);

        void clearRule(tilemap::Tile tile, tilemap::TileEdge edge);

        [[nodiscard]] bool hasNoRuleFor(
            tilemap::Tile tile,
            tilemap::TileEdge edge,
            tilemap::Atlas atlas) const;

        [[nodiscard]] std::optional<bool> corner(
            tilemap::Tile tile, voxel::Corner corner) const;

        void setCorner(
            tilemap::Tile tile,
            voxel::Corner corner,
            std::optional<bool> cornerFilled);

        [[nodiscard]] std::vector<std::pair<voxel::Corner, bool>> cornersOf(
            tilemap::Tile tile) const;

        [[nodiscard]] voxel::Kind kindOf(tilemap::Tile tile) const;

        void setKind(tilemap::Tile tile, voxel::Kind kind);

        [[nodiscard]] std::vector<std::pair<tilemap::Tile, voxel::Kind>> kinds()
            const;

        [[nodiscard]] voxel::Facing facingOf(tilemap::Tile tile) const;

        void setFacing(tilemap::Tile tile, voxel::Facing facing);

        [[nodiscard]] std::vector<std::pair<tilemap::Tile, voxel::Facing>>
        facings()
            const;

        [[nodiscard]] voxel::StairHalf levelOf(tilemap::Tile tile) const;

        void setLevel(tilemap::Tile tile, voxel::StairHalf levelHalf);

        [[nodiscard]] std::vector<std::pair<tilemap::Tile, voxel::StairHalf>>
        levels()
            const;

        [[nodiscard]] voxel::StairPart partOf(tilemap::Tile tile) const;

        void setPart(tilemap::Tile tile, voxel::StairPart part);

        [[nodiscard]] std::vector<std::pair<tilemap::Tile, voxel::StairPart>>
        parts()
            const;

        [[nodiscard]] std::set<tilemap::Tile> allowed(
            tilemap::Tile tile, tilemap::TileEdge edge) const;

        [[nodiscard]] std::vector<TileRule> allRules() const;

        [[nodiscard]] std::size_t size() const;

        [[nodiscard]] bool operator==(const TileRules &other) const
            = default;

    private:
        using Edge = std::pair<tilemap::Tile, tilemap::TileEdge>;

        struct AllowedTiles final
        {
            std::set<tilemap::Tile> tiles{};

            bool air = false;

            [[nodiscard]] bool operator==(const AllowedTiles &other) const
                = default;
        };

        void set(
            tilemap::Tile tile,
            tilemap::TileEdge edge,
            tilemap::Tile neighbourTile,
            bool allowed);

        std::map<Edge, AllowedTiles> allowedByEdge{};

        std::map<std::pair<tilemap::Tile, voxel::Corner>, bool> byCorner{};

        std::map<tilemap::Tile, voxel::Kind> byKind{};

        std::map<tilemap::Tile, voxel::Facing> byFacing{};

        std::map<tilemap::Tile, voxel::StairHalf> halfByLevel{};

        std::map<tilemap::Tile, voxel::StairPart> byPart{};
    };

}
