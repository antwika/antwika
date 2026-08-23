#include "antwika/tile/TileShapes.hpp"

#include <array>
#include <set>
#include <utility>

#include <antwika/enums/Enumeration.hpp>

namespace antwika::tile
{

    namespace
    {
        struct CornerAcross final
        {
            voxel::Corner corner = voxel::Corner::TopLeft;

            voxel::Side answerSide = voxel::Side::Top;
        };

        struct SideCornersRow final
        {
            voxel::Side side;
            std::array<CornerAcross, 2> corners;
        };

        constexpr std::array<SideCornersRow, voxel::kFaceSides>
            kSideCornersRows{{
            {voxel::Side::Top,
             {CornerAcross{voxel::Corner::TopLeft, voxel::Side::Left},
              CornerAcross{voxel::Corner::TopRight, voxel::Side::Right}}},
            {voxel::Side::Bottom,
             {CornerAcross{voxel::Corner::BottomLeft, voxel::Side::Left},
              CornerAcross{voxel::Corner::BottomRight, voxel::Side::Right}}},
            {voxel::Side::Left,
             {CornerAcross{voxel::Corner::TopLeft, voxel::Side::Top},
              CornerAcross{voxel::Corner::BottomLeft, voxel::Side::Bottom}}},
            {voxel::Side::Right,
             {CornerAcross{voxel::Corner::TopRight, voxel::Side::Top},
              CornerAcross{
                  voxel::Corner::BottomRight, voxel::Side::Bottom}}}}};

        static_assert(
            enums::tagsInOrder(kSideCornersRows, &SideCornersRow::side));

        [[nodiscard]] std::array<CornerAcross, 2> cornersAlong(
            const voxel::Side side)
        {
            return enums::lookup(kSideCornersRows, side).corners;
        }

        [[nodiscard]] bool cornersAgree(
            const TileRules &rules,
            const tilemap::Tile tile,
            const tilemap::TileEdge edge,
            const tilemap::Tile otherTile)
        {
            for (const auto &[corner, answerSide] :
                 cornersAlong(edge.side))
            {
                const auto cornerRule = rules.getCorner(tile, corner);

                if (!cornerRule.has_value())
                {
                    continue;
                }

                const auto hasBorderEdge = hasBorder(
                    rules,
                    otherTile,
                    tilemap::TileEdge{.side = answerSide, .edge = edge.edge});

                if (*cornerRule == hasBorderEdge)
                {
                    return false;
                }
            }

            return true;
        }

        [[nodiscard]] std::set<tilemap::Tile> spokenOf(
            const TileRules &rules, const voxel::Kind kind)
        {
            std::set<tilemap::Tile> tiles;

            for (const auto &rule : rules.getAllRules())
            {
                if (rules.kindOf(rule.tile) == kind)
                {
                    tiles.insert(rule.tile);
                }
            }

            return tiles;
        } // GCOVR_EXCL_LINE
    }

    bool hasBorder(
        const TileRules &rules,
        const tilemap::Tile tile,
        const tilemap::TileEdge edge)
    {
        return !rules.hasNoRule(tile, edge)
               && rules.allowsBoundary(tile, edge);
    }

    bool shapesCompatible(
        const TileRules &rules,
        const tilemap::Tile tile,
        const tilemap::TileEdge edge,
        const tilemap::Tile otherTile)
    {
        if (tile.atlas != otherTile.atlas)
        {
            return false;
        }

        if (hasBorder(rules, tile, edge)
            || hasBorder(rules, otherTile, voxel::getFacing(edge)))
        {
            return false;
        }

        return cornersAgree(rules, tile, edge, otherTile)
               && cornersAgree(rules, otherTile, voxel::getFacing(edge), tile);
    }

    ShapedJunctions getRulesFromShapes(
        const TileRules &rules, const voxel::Kind kind)
    {
        const auto tiles = spokenOf(rules, kind);

        ShapedJunctions foundJunctions;

        for (const auto tile : tiles)
        {
            for (const auto edge : tilemap::kEveryTileEdge)
            {
                const auto allowedTiles = rules.getAllowed(tile, edge);

                for (const auto other : tiles)
                {
                    const auto shapesFit =
                        shapesCompatible(rules, tile, edge, other);
                    const auto tileAllowed = allowedTiles.contains(other);

                    if (shapesFit == tileAllowed)
                    {
                        continue;
                    }

                    (shapesFit ? foundJunctions.toAddRules
                            : foundJunctions.conflictingRules)
                        .push_back(
                            TileRule{
                                .tile = tile,
                                .edge = edge,
                                .allowedTiles = {other},
                                .allowsBoundary =
                                    rules.allowsBoundary(
                                        tile, edge)});
                }
            }
        }

        return foundJunctions;
    } // GCOVR_EXCL_LINE

}
