#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/tile/TileRules.hpp>

namespace antwika::tile
{

    namespace
    {
        constexpr tilemap::Tile kGrassTile{
            .atlas = tilemap::Atlas::Floor, .index = 3};

        constexpr tilemap::Tile kWallTile{
            .atlas = tilemap::Atlas::Wall, .index = 7};

        constexpr tilemap::Tile kWaterTile{
            .atlas = tilemap::Atlas::Floor, .index = 9};

        constexpr tilemap::TileEdge kOutwardEdge{
            .side = voxel::Side::Top, .edge = voxel::EdgeKind::Boundary};

        constexpr tilemap::TileEdge kInwardEdge{
            .side = voxel::Side::Top, .edge = voxel::EdgeKind::Interior};

        constexpr tilemap::TileEdge kBelowEdge{
            .side = voxel::Side::Bottom, .edge = voxel::EdgeKind::Boundary};

        TEST(TileRulesTest, Allows_ForbidsEveryNeighbourUntilToldOtherwise)
        {
            const tile::TileRules rules;

            EXPECT_FALSE(
                rules.allows(kGrassTile, kOutwardEdge, kWallTile));
            EXPECT_EQ(rules.size(), 0U);
        }

        TEST(TileRulesTest, Toggle_AllowsANeighbourItIsToldOf)
        {
            tile::TileRules rules;

            rules.toggle(kGrassTile, kOutwardEdge, kWallTile);

            EXPECT_TRUE(
                rules.allows(kGrassTile, kOutwardEdge, kWallTile));
        }

        TEST(TileRulesTest, Toggle_ForbidsANeighbourToldOfTwice)
        {
            tile::TileRules rules;

            rules.toggle(kGrassTile, kOutwardEdge, kWallTile);
            rules.toggle(kGrassTile, kOutwardEdge, kWallTile);

            EXPECT_FALSE(
                rules.allows(kGrassTile, kOutwardEdge, kWallTile));
            EXPECT_EQ(rules.size(), 0U);
        }

        TEST(TileRulesTest, SetAllows_SaysWhatItIsAskedAtBothEnds)
        {
            tile::TileRules rules;

            rules.setAllows(kGrassTile, kOutwardEdge, kWallTile, true);

            EXPECT_TRUE(rules.allows(kGrassTile, kOutwardEdge, kWallTile));
            EXPECT_TRUE(
                rules.allows(
                    kWallTile,
                    voxel::facing(kOutwardEdge),
                    kGrassTile));
        }

        TEST(TileRulesTest, SetAllows_SaysNoMoreForBeingAskedTwice)
        {
            tile::TileRules rules;

            rules.setAllows(kGrassTile, kOutwardEdge, kWallTile, true);
            rules.setAllows(kGrassTile, kOutwardEdge, kWallTile, true);

            EXPECT_TRUE(rules.allows(kGrassTile, kOutwardEdge, kWallTile));
            EXPECT_EQ(rules.size(), 2U);
        }

        TEST(TileRulesTest, SetAllows_TakesBothEndsBackWhenForbidding)
        {
            tile::TileRules rules;

            rules.setAllows(kGrassTile, kOutwardEdge, kWallTile, true);
            rules.setAllows(kGrassTile, kOutwardEdge, kWallTile, false);

            EXPECT_FALSE(rules.allows(kGrassTile, kOutwardEdge, kWallTile));
            EXPECT_FALSE(
                rules.allows(
                    kWallTile,
                    voxel::facing(kOutwardEdge),
                    kGrassTile));
            EXPECT_EQ(rules.size(), 0U);
        }

        TEST(TileRulesTest, Toggle_KeepsTheEdgesOfATileApart)
        {
            tile::TileRules rules;

            rules.toggle(kGrassTile, kOutwardEdge, kWallTile);

            EXPECT_FALSE(rules.allows(kGrassTile, kInwardEdge, kWallTile));
        }

        TEST(TileRulesTest, Toggle_KeepsTheSidesOfATileApart)
        {
            tile::TileRules rules;

            rules.toggle(kGrassTile, kOutwardEdge, kWallTile);

            EXPECT_FALSE(rules.allows(kGrassTile, kBelowEdge, kWallTile));
        }

        TEST(TileRulesTest, Toggle_HoldsARuleForEveryEdgeAtOnce)
        {
            tile::TileRules rules;

            for (const auto edge : tilemap::kEveryTileEdge)
            {
                rules.toggle(kGrassTile, edge, kWallTile);
            }

            EXPECT_EQ(rules.size(), tilemap::kEveryTileEdge.size() * 2);

            for (const auto edge : tilemap::kEveryTileEdge)
            {
                EXPECT_TRUE(rules.allows(kGrassTile, edge, kWallTile));
            }
        }

        TEST(TileRulesTest, Toggle_KeepsTheRulesOfTwoTilesApart)
        {
            tile::TileRules rules;

            rules.toggle(kGrassTile, kOutwardEdge, kWallTile);

            EXPECT_FALSE(
                rules.allows(kWaterTile, kOutwardEdge, kWallTile));
        }

        TEST(TileRulesTest, Toggle_TellsATileFromAnotherOfTheSameIndex)
        {
            tile::TileRules rules;
            constexpr tilemap::Tile flatTile{
                .atlas = tilemap::Atlas::Floor, .index = 5};
            constexpr tilemap::Tile uprightTile{
                .atlas = tilemap::Atlas::Wall, .index = 5};

            rules.toggle(flatTile, kOutwardEdge, kWallTile);

            EXPECT_TRUE(rules.allows(flatTile, kOutwardEdge, kWallTile));
            EXPECT_FALSE(
                rules.allows(uprightTile, kOutwardEdge, kWallTile));
        }

        TEST(TileRulesTest, Allowed_HoldsEveryNeighbourAnEdgeIsGiven)
        {
            tile::TileRules rules;

            rules.toggle(kGrassTile, kOutwardEdge, kWallTile);
            rules.toggle(kGrassTile, kOutwardEdge, kWaterTile);

            EXPECT_THAT(
                rules.allowed(kGrassTile, kOutwardEdge),
                testing::UnorderedElementsAre(kWallTile, kWaterTile));
            EXPECT_EQ(rules.size(), 4U);
        }

        TEST(TileRulesTest, Allowed_HoldsNothingForAnEdgeNeverSpokenOf)
        {
            const tile::TileRules rules;

            EXPECT_THAT(
                rules.allowed(kGrassTile, kInwardEdge),
                testing::IsEmpty());
        }

        TEST(TileRulesTest, Toggle_SaysItOfTheNeighbourToo)
        {
            tile::TileRules rules;

            rules.toggle(
                kGrassTile,
                kOutwardEdge,
                kWallTile);

            EXPECT_TRUE(
                rules.allows(
                    kWallTile,
                    voxel::facing(kOutwardEdge),
                    kGrassTile));
        }

        TEST(TileRulesTest, Toggle_TakesBothEndsBackAgainTogether)
        {
            tile::TileRules rules;

            rules.toggle(kGrassTile, kOutwardEdge, kWallTile);
            rules.toggle(kGrassTile, kOutwardEdge, kWallTile);

            EXPECT_FALSE(rules.allows(kGrassTile, kOutwardEdge, kWallTile));
            EXPECT_FALSE(
                rules.allows(
                    kWallTile, voxel::facing(kOutwardEdge), kGrassTile));
            EXPECT_EQ(rules.size(), 0U);
        }

        TEST(TileRulesTest, Toggle_SquaresUpAnEndLaidOnItsOwn)
        {
            tile::TileRules rules;

            rules.allow(kGrassTile, kOutwardEdge, kWallTile);
            rules.toggle(kGrassTile, kOutwardEdge, kWallTile);

            EXPECT_FALSE(rules.allows(kGrassTile, kOutwardEdge, kWallTile));
            EXPECT_FALSE(
                rules.allows(
                    kWallTile, voxel::facing(kOutwardEdge), kGrassTile));
        }

        TEST(TileRulesTest, Toggle_SquaresUpTheFarEndLaidOnItsOwn)
        {
            tile::TileRules rules;

            rules.allow(kWallTile, voxel::facing(kOutwardEdge), kGrassTile);
            rules.toggle(kGrassTile, kOutwardEdge, kWallTile);

            EXPECT_TRUE(rules.allows(kGrassTile, kOutwardEdge, kWallTile));
            EXPECT_TRUE(
                rules.allows(
                    kWallTile, voxel::facing(kOutwardEdge), kGrassTile));
        }

        TEST(TileRulesTest, Allow_SaysNothingOfTheNeighbour)
        {
            tile::TileRules rules;

            rules.allow(kGrassTile, kOutwardEdge, kWallTile);

            EXPECT_TRUE(rules.allows(kGrassTile, kOutwardEdge, kWallTile));
            EXPECT_FALSE(
                rules.allows(
                    kWallTile, voxel::facing(kOutwardEdge), kGrassTile));
        }

        TEST(TileRulesTest, Toggle_LeavesTheBoundaryEdgesAloneEntirely)
        {
            tile::TileRules rules;

            for (const auto side :
                 {voxel::Side::Top,
                  voxel::Side::Bottom,
                  voxel::Side::Left,
                  voxel::Side::Right})
            {
                rules.toggle(
                    kGrassTile,
                    tilemap::TileEdge{
                        .side = side, .edge = voxel::EdgeKind::Interior},
                    kWallTile);
            }

            for (const auto &rule : rules.allRules())
            {
                EXPECT_EQ(rule.edge.edge, voxel::EdgeKind::Interior);
            }

            EXPECT_EQ(rules.size(), 8U);
        }

        TEST(TileRulesTest, Toggle_KeepsTheKindOfEdgeItIsGiven)
        {
            for (const auto edge : tilemap::kEveryTileEdge)
            {
                tile::TileRules rules;

                rules.toggle(kGrassTile, edge, kWallTile);

                for (const auto &rule : rules.allRules())
                {
                    EXPECT_EQ(rule.edge.edge, edge.edge);
                }
            }
        }

        TEST(TileRulesTest, Toggle_LetsATileStandAgainstItself)
        {
            tile::TileRules rules;

            rules.toggle(kGrassTile, kOutwardEdge, kGrassTile);

            EXPECT_TRUE(
                rules.allows(kGrassTile, kOutwardEdge, kGrassTile));
        }

        TEST(TileRulesTest, Toggle_KeepsARuleWhileAnotherOfTheEdgeGoes)
        {
            tile::TileRules rules;

            rules.toggle(kGrassTile, kOutwardEdge, kWallTile);
            rules.toggle(kGrassTile, kOutwardEdge, kWaterTile);
            rules.toggle(kGrassTile, kOutwardEdge, kWallTile);

            EXPECT_THAT(
                rules.allowed(kGrassTile, kOutwardEdge),
                testing::UnorderedElementsAre(kWaterTile));
        }

        TEST(TileRulesTest, ForbidAll_LeavesAnEdgeAllowingNothing)
        {
            tile::TileRules rules;

            rules.forbidAll(kGrassTile, kOutwardEdge);

            EXPECT_TRUE(rules.isForbidden(kGrassTile, kOutwardEdge));
            EXPECT_FALSE(rules.hasNoRule(kGrassTile, kOutwardEdge));
            EXPECT_FALSE(rules.allows(kGrassTile, kOutwardEdge, kWallTile));
            EXPECT_THAT(
                rules.allowed(kGrassTile, kOutwardEdge), testing::IsEmpty());
        }

        TEST(TileRulesTest, ForbidAll_HasARuleForEitherAtlas)
        {
            tile::TileRules rules;

            rules.forbidAll(kGrassTile, kOutwardEdge);

            EXPECT_FALSE(
                rules.hasNoRuleFor(
                    kGrassTile,
                    kOutwardEdge,
                    tilemap::Atlas::Floor));
            EXPECT_FALSE(
                rules.hasNoRuleFor(
                    kGrassTile,
                    kOutwardEdge,
                    tilemap::Atlas::Wall));
        }

        TEST(TileRulesTest, ForbidAll_ForbidsThatEdgeAndNoOther)
        {
            tile::TileRules rules;

            rules.forbidAll(kGrassTile, kOutwardEdge);

            EXPECT_FALSE(rules.isForbidden(kGrassTile, kInwardEdge));
            EXPECT_TRUE(rules.hasNoRule(kGrassTile, kInwardEdge));
            EXPECT_FALSE(rules.isForbidden(kWallTile, kOutwardEdge));
        }

        TEST(TileRulesTest, ForbidAll_ThrowsNoRuleAwayOnTheFarSide)
        {
            tile::TileRules rules;

            rules.toggle(kGrassTile, kOutwardEdge, kWallTile);
            rules.forbidAll(
                kGrassTile,
                kOutwardEdge);

            EXPECT_TRUE(
                rules.allows(
                    kWallTile, voxel::facing(kOutwardEdge), kGrassTile));
        }

        TEST(TileRulesTest, ClearRule_LeavesAnEdgeSilentAgain)
        {
            tile::TileRules rules;

            rules.forbidAll(kGrassTile, kOutwardEdge);
            rules.clearRule(kGrassTile, kOutwardEdge);

            EXPECT_TRUE(rules.hasNoRule(kGrassTile, kOutwardEdge));
            EXPECT_FALSE(rules.isForbidden(kGrassTile, kOutwardEdge));
        }

        TEST(TileRulesTest, Toggle_OpensAForbiddenEdgeToWhatItIsGiven)
        {
            tile::TileRules rules;

            rules.forbidAll(kGrassTile, kOutwardEdge);
            rules.toggle(kGrassTile, kOutwardEdge, kWallTile);

            EXPECT_FALSE(rules.isForbidden(kGrassTile, kOutwardEdge));
            EXPECT_TRUE(rules.allows(kGrassTile, kOutwardEdge, kWallTile));
        }

    
        TEST(
            TileRulesTest,
            SetAllowsBoundary_ShutsAnEdgeNamingNoTileAgainstTiles)
        {
            tile::TileRules rules;

            rules.setAllowsBoundary(kGrassTile, kOutwardEdge, true);

            EXPECT_TRUE(
                rules.allowsBoundary(kGrassTile, kOutwardEdge));
            EXPECT_FALSE(
                rules.allows(kGrassTile, kOutwardEdge, kWallTile));
            EXPECT_FALSE(
                rules.hasNoRuleFor(
                    kGrassTile,
                    kOutwardEdge,
                    tilemap::Atlas::Floor));
            EXPECT_FALSE(
                rules.hasNoRuleFor(
                    kGrassTile,
                    kOutwardEdge,
                    tilemap::Atlas::Wall));
        }

        TEST(TileRulesTest, SetAllowsBoundary_LeavesATileEdgeSayingBoth)
        {
            tile::TileRules rules;

            rules.allow(kGrassTile, kOutwardEdge, kWallTile);
            rules.setAllowsBoundary(kGrassTile, kOutwardEdge, true);

            EXPECT_TRUE(rules.allowsBoundary(kGrassTile, kOutwardEdge));
            EXPECT_TRUE(rules.allows(kGrassTile, kOutwardEdge, kWallTile));
            EXPECT_FALSE(rules.allows(kGrassTile, kOutwardEdge, kWaterTile));
        }

        TEST(TileRulesTest, AllowsBoundary_LetsASilentEdgeLieAtTheRim)
        {
            const tile::TileRules rules;

            EXPECT_TRUE(rules.allowsBoundary(kGrassTile, kOutwardEdge));
        }

        TEST(TileRulesTest, AllowsBoundary_KeepsAForbiddenEdgeFromTheRim)
        {
            tile::TileRules rules;

            rules.forbidAll(kGrassTile, kOutwardEdge);

            EXPECT_FALSE(rules.allowsBoundary(kGrassTile, kOutwardEdge));
        }

    
        TEST(TileRulesTest, Corner_AsksNothingOfACornerUntilTold)
        {
            const tile::TileRules rules;

            EXPECT_FALSE(
                rules.corner(kGrassTile, voxel::Corner::TopLeft).has_value());
            EXPECT_TRUE(rules.cornersOf(kGrassTile).empty());
        }

        TEST(TileRulesTest, SetCorner_AsksACornerBeFilledOrEmpty)
        {
            tile::TileRules rules;

            rules.setCorner(kGrassTile, voxel::Corner::TopLeft, true);
            rules.setCorner(kGrassTile, voxel::Corner::BottomRight, false);

            EXPECT_EQ(rules.corner(kGrassTile, voxel::Corner::TopLeft), true);
            EXPECT_EQ(
                rules.corner(kGrassTile, voxel::Corner::BottomRight), false);
            EXPECT_EQ(rules.cornersOf(kGrassTile).size(), 2U);
        }

        TEST(TileRulesTest, SetCorner_AsksNothingOfItAgain)
        {
            tile::TileRules rules;

            rules.setCorner(kGrassTile, voxel::Corner::TopLeft, true);
            rules.setCorner(kGrassTile, voxel::Corner::TopLeft, std::nullopt);

            EXPECT_FALSE(
                rules.corner(kGrassTile, voxel::Corner::TopLeft).has_value());
            EXPECT_TRUE(rules.cornersOf(kGrassTile).empty());
        }

        TEST(TileRulesTest, SetCorner_KeepsTheCornersOfTwoTilesApart)
        {
            tile::TileRules rules;

            rules.setCorner(kGrassTile, voxel::Corner::TopLeft, true);

            EXPECT_FALSE(
                rules.corner(kWallTile, voxel::Corner::TopLeft).has_value());
        }

        TEST(TileRulesTest, SetCorner_LeavesTheEdgesOfATileAlone)
        {
            tile::TileRules rules;

            rules.toggle(kGrassTile, kOutwardEdge, kWallTile);
            rules.setCorner(kGrassTile, voxel::Corner::TopLeft, false);

            EXPECT_TRUE(rules.allows(kGrassTile, kOutwardEdge, kWallTile));
        }

        }
    TEST(TileRulesTest, KindOf_MakesEveryTileASolidOneUntilItIsGiven)
    {
        const tile::TileRules rules;

        EXPECT_EQ(rules.kindOf(tilemap::Tile{}), voxel::Kind::Normal);
    }

    TEST(TileRulesTest, SetKind_GivesATileToAKind)
    {
        tile::TileRules rules;
        const tilemap::Tile tile{.atlas = tilemap::Atlas::Floor, .index = 3};

        rules.setKind(tile, voxel::Kind::Water);

        EXPECT_EQ(rules.kindOf(tile), voxel::Kind::Water);
        EXPECT_EQ(rules.kindOf(tilemap::Tile{}), voxel::Kind::Normal);
    }

    TEST(TileRulesTest, SetKind_SaysNothingOfATileGivenBackToASolidOne)
    {
        tile::TileRules rules;
        const tilemap::Tile tile{.atlas = tilemap::Atlas::Floor, .index = 3};

        rules.setKind(tile, voxel::Kind::Ramp);
        rules.setKind(tile, voxel::Kind::Normal);

        EXPECT_EQ(rules.kindOf(tile), voxel::Kind::Normal);
        EXPECT_TRUE(rules.kinds().empty());
    }

    TEST(TileRulesTest, Kinds_ReadsOutInAnOrderTheTilesSettle)
    {
        tile::TileRules rules;
        const tilemap::Tile laterTile{
            .atlas = tilemap::Atlas::Floor,
            .index = 9};
        const tilemap::Tile soonerTile{
            .atlas = tilemap::Atlas::Wall,
            .index = 1};

        rules.setKind(laterTile, voxel::Kind::Water);
        rules.setKind(soonerTile, voxel::Kind::Ramp);

        const auto entries = rules.kinds();

        ASSERT_EQ(entries.size(), 2U);
        EXPECT_EQ(entries.front().first, soonerTile);
        EXPECT_EQ(entries.front().second, voxel::Kind::Ramp);
        EXPECT_EQ(entries.back().first, laterTile);
    }
    TEST(TileRulesTest, LevelOf_DrawsATileForEitherUntilItIsSaid)
    {
        const tile::TileRules rules;

        EXPECT_EQ(rules.levelOf(tilemap::Tile{}), voxel::StairHalf::Any);
    }

    TEST(TileRulesTest, SetLevel_DrawsATileForOneLevelOfAFlight)
    {
        tile::TileRules rules;
        const tilemap::Tile tile{.atlas = tilemap::Atlas::Wall, .index = 3};

        rules.setLevel(tile, voxel::StairHalf::Lower);

        EXPECT_EQ(rules.levelOf(tile), voxel::StairHalf::Lower);
        EXPECT_EQ(rules.levelOf(tilemap::Tile{}), voxel::StairHalf::Any);
    }

    TEST(TileRulesTest, SetLevel_SaysNothingOfATileDrawnForEither)
    {
        tile::TileRules rules;
        const tilemap::Tile tile{.atlas = tilemap::Atlas::Wall, .index = 3};

        rules.setLevel(tile, voxel::StairHalf::Upper);
        rules.setLevel(tile, voxel::StairHalf::Any);

        EXPECT_EQ(rules.levelOf(tile), voxel::StairHalf::Any);
        EXPECT_TRUE(rules.levels().empty());
    }

    TEST(TileRulesTest, PartOf_DrawsATileForEitherPartUntilItIsSaid)
    {
        const tile::TileRules rules;

        EXPECT_EQ(
            rules.partOf(tilemap::Tile{}), antwika::voxel::StairPart::Any);
    }

    TEST(TileRulesTest, SetPart_DrawsATileForOnePartOfAFlight)
    {
        tile::TileRules rules;
        const tilemap::Tile tile{.atlas = tilemap::Atlas::Wall, .index = 3};

        rules.setPart(tile, antwika::voxel::StairPart::Side);

        EXPECT_EQ(
            rules.partOf(tile), antwika::voxel::StairPart::Side);
        EXPECT_EQ(
            rules.partOf(tilemap::Tile{}), antwika::voxel::StairPart::Any);
    }

    TEST(TileRulesTest, SetPart_SaysNothingOfATileDrawnForEither)
    {
        tile::TileRules rules;
        const tilemap::Tile tile{.atlas = tilemap::Atlas::Wall, .index = 3};

        rules.setPart(tile, antwika::voxel::StairPart::Front);
        rules.setPart(tile, antwika::voxel::StairPart::Any);

        EXPECT_EQ(
            rules.partOf(tile), antwika::voxel::StairPart::Any);
        EXPECT_TRUE(rules.parts().empty());
    }

    TEST(TileRulesTest, FacingOf_DrawsATileForAnyFlightUntilItIsSaid)
    {
        const tile::TileRules rules;

        EXPECT_EQ(rules.facingOf(tilemap::Tile{}), voxel::Facing::Any);
    }

    TEST(TileRulesTest, SetFacing_DrawsATileForOneWayAbout)
    {
        tile::TileRules rules;
        const tilemap::Tile tile{.atlas = tilemap::Atlas::Wall, .index = 115};

        rules.setFacing(tile, voxel::Facing::West);

        EXPECT_EQ(rules.facingOf(tile), voxel::Facing::West);
        EXPECT_EQ(rules.facingOf(tilemap::Tile{}), voxel::Facing::Any);
    }

    TEST(TileRulesTest, SetFacing_SaysNothingOfATileDrawnForAnyFlight)
    {
        tile::TileRules rules;
        const tilemap::Tile tile{.atlas = tilemap::Atlas::Wall, .index = 115};

        rules.setFacing(tile, voxel::Facing::East);
        rules.setFacing(tile, voxel::Facing::Any);

        EXPECT_EQ(rules.facingOf(tile), voxel::Facing::Any);
        EXPECT_TRUE(rules.facings().empty());
    }

    TEST(TileRulesTest, BoundaryOnly_HoldsAnEdgeMeetingOnlyTheBoundary)
    {
        tile::TileRules rules;
        const tilemap::Tile tile{.atlas = tilemap::Atlas::Wall, .index = 0};
        const tilemap::TileEdge edge{
            .side = voxel::Side::Top, .edge = voxel::EdgeKind::Interior};

        rules.setAllowsBoundary(tile, edge, true);

        EXPECT_TRUE(rules.boundaryOnly(tile, edge));
    }

    TEST(TileRulesTest, BoundaryOnly_LetsGoOfAnEdgeATileMayMeet)
    {
        tile::TileRules rules;
        const tilemap::Tile tile{.atlas = tilemap::Atlas::Wall, .index = 0};
        const tilemap::Tile otherTile{
            .atlas = tilemap::Atlas::Wall,
            .index = 1};
        const tilemap::TileEdge edge{
            .side = voxel::Side::Top, .edge = voxel::EdgeKind::Interior};

        rules.setAllowsBoundary(tile, edge, true);
        rules.allow(tile, edge, otherTile);

        EXPECT_FALSE(rules.boundaryOnly(tile, edge));
    }

    TEST(TileRulesTest, BoundaryOnly_SaysNothingOfASilentEdge)
    {
        const tile::TileRules rules;
        const tilemap::Tile tile{.atlas = tilemap::Atlas::Wall, .index = 0};

        EXPECT_FALSE(
            rules.boundaryOnly(
                tile,
                tilemap::TileEdge{
                    .side = voxel::Side::Top,
                    .edge = voxel::EdgeKind::Interior}));
    }

    TEST(TileRulesTest, BoundaryOnly_TellsAForbiddenEdgeFromABorderedOne)
    {
        tile::TileRules rules;
        const tilemap::Tile tile{.atlas = tilemap::Atlas::Wall, .index = 0};
        const tilemap::TileEdge edge{
            .side = voxel::Side::Top, .edge = voxel::EdgeKind::Interior};

        rules.forbidAll(tile, edge);

        EXPECT_TRUE(rules.isForbidden(tile, edge));
        EXPECT_FALSE(rules.boundaryOnly(tile, edge));
    }

}
