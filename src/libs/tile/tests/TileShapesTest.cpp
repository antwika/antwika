#include <gtest/gtest.h>

#include <algorithm>

#include <antwika/tile/TileShapes.hpp>

namespace antwika::tile
{

    namespace
    {
        constexpr tilemap::Tile kFieldTile{
            .atlas = tilemap::Atlas::Floor, .index = 3};

        constexpr tilemap::Tile kOtherTile{
            .atlas = tilemap::Atlas::Floor, .index = 4};

        constexpr tilemap::Tile kWallTile{
            .atlas = tilemap::Atlas::Wall, .index = 7};

        constexpr tilemap::TileEdge kRightOutEdge{
            .side = voxel::Side::Right, .edge = voxel::EdgeKind::Boundary};

        constexpr tilemap::TileEdge kTopOutEdge{
            .side = voxel::Side::Top, .edge = voxel::EdgeKind::Boundary};

        [[nodiscard]] tile::TileRules borderedAt(
            const tilemap::Tile tile, const tilemap::TileEdge edge)
        {
            tile::TileRules rules;

            rules.setAllowsBoundary(tile, edge, true);

            return rules;
        }

        [[nodiscard]] bool names(
            const std::vector<tile::TileRule> &rules,
            const tilemap::Tile tile,
            const tilemap::TileEdge edge,
            const tilemap::Tile otherTile)
        {
            return std::ranges::any_of(
                rules,
                [&](const tile::TileRule &rule)
                {
                    return rule.tile == tile && rule.edge == edge
                           && rule.allowedTiles.contains(otherTile);
                });
        }

        TEST(TileShapesTest, HasBorder_TakesSilenceForNoOpinion)
        {
            const tile::TileRules rules;

            EXPECT_TRUE(rules.allowsBoundary(kFieldTile, kRightOutEdge));
            EXPECT_FALSE(tile::hasBorder(rules, kFieldTile, kRightOutEdge));
        }

        TEST(TileShapesTest, HasBorder_HoldsAnEdgeSaidToLieAtARim)
        {
            const auto rules = borderedAt(kFieldTile, kRightOutEdge);

            EXPECT_TRUE(tile::hasBorder(rules, kFieldTile, kRightOutEdge));
        }

        TEST(
            TileShapesTest,
            HasBorder_LetsGoOfAnEdgeShutAgainstEverything)
        {
            tile::TileRules rules;

            rules.forbidAll(kFieldTile, kRightOutEdge);

            EXPECT_FALSE(tile::hasBorder(rules, kFieldTile, kRightOutEdge));
        }

        TEST(TileShapesTest, ShapesCompatible_LetsTwoPlainEdgesMeet)
        {
            const tile::TileRules rules;

            EXPECT_TRUE(
                tile::shapesCompatible(
                    rules,
                    kFieldTile,
                    kRightOutEdge,
                    kOtherTile));
        }

        TEST(
            TileShapesTest,
            ShapesCompatible_KeepsABorderedEdgeFromMeetingAnything)
        {
            const auto mine = borderedAt(kFieldTile, kRightOutEdge);

            EXPECT_FALSE(
                tile::shapesCompatible(
                    mine,
                    kFieldTile,
                    kRightOutEdge,
                    kOtherTile));

            const auto theirs =
                borderedAt(kOtherTile, voxel::facing(kRightOutEdge));

            EXPECT_FALSE(
                tile::shapesCompatible(
                    theirs,
                    kFieldTile,
                    kRightOutEdge,
                    kOtherTile));
        }

        TEST(TileShapesTest, ShapesCompatible_KeepsToOneAtlas)
        {
            const tile::TileRules rules;

            EXPECT_FALSE(
                tile::shapesCompatible(
                    rules, kFieldTile, kRightOutEdge, kWallTile));
        }

        TEST(TileShapesTest, ShapesCompatible_HoldsACornerToTheEdgeBeyondIt)
        {
            tile::TileRules askingRules;

            askingRules.setCorner(kFieldTile, voxel::Corner::TopRight, true);

            EXPECT_TRUE(
                tile::shapesCompatible(
                    askingRules,
                    kFieldTile,
                    kRightOutEdge,
                    kOtherTile));

            askingRules.setAllowsBoundary(kOtherTile, kTopOutEdge, true);

            EXPECT_FALSE(
                tile::shapesCompatible(
                    askingRules,
                    kFieldTile,
                    kRightOutEdge,
                    kOtherTile));
        }

        TEST(
            TileShapesTest,
            ShapesCompatible_HoldsACornerAskingForNothingBeyondIt)
        {
            tile::TileRules askingRules;

            askingRules.setCorner(kFieldTile, voxel::Corner::TopRight, false);

            EXPECT_FALSE(
                tile::shapesCompatible(
                    askingRules,
                    kFieldTile,
                    kRightOutEdge,
                    kOtherTile));

            askingRules.setAllowsBoundary(kOtherTile, kTopOutEdge, true);

            EXPECT_TRUE(
                tile::shapesCompatible(
                    askingRules,
                    kFieldTile,
                    kRightOutEdge,
                    kOtherTile));
        }

        TEST(TileShapesTest, ShapesCompatible_SaysTheSameFromEitherEnd)
        {
            tile::TileRules rules;

            rules.setAllowsBoundary(kFieldTile, kTopOutEdge, true);
            rules.setCorner(kOtherTile, voxel::Corner::TopLeft, true);
            rules.setCorner(kFieldTile, voxel::Corner::BottomRight, false);

            for (const auto edge : tilemap::kEveryTileEdge)
            {
                EXPECT_EQ(
                    tile::shapesCompatible(
                        rules, kFieldTile, edge, kOtherTile),
                    tile::shapesCompatible(
                        rules, kOtherTile, voxel::facing(edge), kFieldTile));
            }
        }

        TEST(TileShapesTest, RulesFromShapes_KeepsToOneKind)
        {
            tile::TileRules rules;

            rules.setAllowsBoundary(kFieldTile, kTopOutEdge, true);
            rules.setAllowsBoundary(kOtherTile, kTopOutEdge, true);
            rules.setKind(kOtherTile, voxel::Kind::Water);

            const auto shapedRules =
                tile::rulesFromShapes(rules, voxel::Kind::Normal);

            EXPECT_FALSE(shapedRules.toAddRules.empty());

            for (const auto &rule : shapedRules.toAddRules)
            {
                EXPECT_EQ(rules.kindOf(rule.tile), voxel::Kind::Normal);

                for (const auto tile : rule.allowedTiles)
                {
                    EXPECT_EQ(rules.kindOf(tile), voxel::Kind::Normal);
                }
            }
        }

        TEST(
            TileShapesTest,
            RulesFromShapes_AsksForNothingAlreadySaid)
        {
            tile::TileRules rules;

            rules.setAllowsBoundary(kFieldTile, kTopOutEdge, true);
            rules.setAllowsBoundary(kOtherTile, kTopOutEdge, true);

            for (const auto &rule :
                 tile::rulesFromShapes(rules, voxel::Kind::Normal).toAddRules)
            {
                for (const auto tile : rule.allowedTiles)
                {
                    rules.allow(rule.tile, rule.edge, tile);
                }
            }

            const auto secondRules =
                tile::rulesFromShapes(rules, voxel::Kind::Normal);

            EXPECT_TRUE(secondRules.toAddRules.empty());
            EXPECT_TRUE(secondRules.conflictingRules.empty());
        }

        TEST(
            TileShapesTest,
            RulesFromShapes_NamesAJunctionTheShapesWillNotHave)
        {
            tile::TileRules rules;

            rules.setAllowsBoundary(kFieldTile, kTopOutEdge, true);
            rules.setAllowsBoundary(kOtherTile, kTopOutEdge, true);
            rules.allow(kFieldTile, kTopOutEdge, kOtherTile);

            const auto shapedRules =
                tile::rulesFromShapes(rules, voxel::Kind::Normal);

            EXPECT_TRUE(
                names(
                    shapedRules.conflictingRules,
                    kFieldTile,
                    kTopOutEdge,
                    kOtherTile));
            EXPECT_FALSE(
                names(shapedRules.toAddRules, kFieldTile, kTopOutEdge,
                kOtherTile));
        }
    }

}
