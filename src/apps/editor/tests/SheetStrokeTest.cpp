#include <gtest/gtest.h>

#include <antwika/editor/editor/state/SheetStroke.hpp>
#include <antwika/tile/TileRules.hpp>
#include <antwika/tilemap/Tilemap.hpp>
#include <antwika/voxel/VoxelCube.hpp>

using antwika::editor::EdgeSelection;
using antwika::editor::SheetStroke;
using antwika::tile::TileRules;
using antwika::tilemap::Atlas;
using antwika::tilemap::Tile;
using antwika::tilemap::TileEdge;
using antwika::voxel::EdgeKind;
using antwika::voxel::Side;

namespace
{
    constexpr Tile kOneTile{.atlas = Atlas::Wall, .index = 1};

    constexpr Tile kOtherTile{.atlas = Atlas::Wall, .index = 2};

    constexpr TileEdge kTopBoundaryEdge{
        .side = Side::Top, .edge = EdgeKind::Boundary};

    [[nodiscard]] SheetStroke getStrokeOnTopEdge()
    {
        SheetStroke sheetStroke;

        sheetStroke.selectedTile = kOneTile;
        sheetStroke.selectedEdges =
            EdgeSelection{.side = Side::Top, .edge = EdgeKind::Boundary};

        return sheetStroke;
    }
}

TEST(SheetStrokeTest, IsForbidden_HoldsForAnEdgeShutToEverything)
{
    TileRules rules;

    rules.forbidAll(kOneTile, kTopBoundaryEdge);

    EXPECT_TRUE(getStrokeOnTopEdge().isForbidden(rules));
}

TEST(SheetStrokeTest, IsForbidden_LeavesAnEdgeThatMeetsSomething)
{
    TileRules rules;

    rules.allow(kOneTile, kTopBoundaryEdge, kOtherTile);

    EXPECT_FALSE(getStrokeOnTopEdge().isForbidden(rules));
}

TEST(SheetStrokeTest, Allows_HoldsForTheNeighbourTheRulesLetIn)
{
    TileRules rules;

    rules.allow(kOneTile, kTopBoundaryEdge, kOtherTile);

    EXPECT_TRUE(getStrokeOnTopEdge().allows(rules, kOtherTile));
}

TEST(SheetStrokeTest, AllowsBoundary_FollowsWhatTheRulesWereTold)
{
    TileRules rules;

    rules.setAllowsBoundary(kOneTile, kTopBoundaryEdge, true);

    EXPECT_TRUE(getStrokeOnTopEdge().allowsBoundary(rules));
}

TEST(SheetStrokeTest, IsForbidden_LeavesAStrokeWithNoTilePicked)
{
    TileRules rules;
    SheetStroke sheetStroke;

    sheetStroke.selectedEdges =
        EdgeSelection{.side = Side::Top, .edge = EdgeKind::Boundary};

    rules.forbidAll(kOneTile, kTopBoundaryEdge);

    EXPECT_FALSE(sheetStroke.isForbidden(rules));
    EXPECT_FALSE(sheetStroke.allowsBoundary(rules));
    EXPECT_FALSE(sheetStroke.allows(rules, kOtherTile));
}

TEST(SheetStrokeTest, IsForbidden_LeavesAStrokeWithNoEdgePicked)
{
    TileRules rules;
    SheetStroke sheetStroke;

    sheetStroke.selectedTile = kOneTile;

    rules.forbidAll(kOneTile, kTopBoundaryEdge);

    EXPECT_FALSE(sheetStroke.isForbidden(rules));
}
