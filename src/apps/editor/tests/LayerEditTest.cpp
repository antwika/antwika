#include <gtest/gtest.h>

#include <antwika/editor/LayerEdit.hpp>
#include <antwika/map/Layers.hpp>
#include <antwika/map/MapFile.hpp>
#include <antwika/tilemap/TileEdges.hpp>
#include <antwika/tilemap/Tilemap.hpp>
#include <antwika/voxel/VoxelCube.hpp>

using antwika::editor::AssignMode;
using antwika::editor::getActiveRules;
using antwika::editor::getEditedTile;
using antwika::editor::isAdjoining;
using antwika::editor::isDecorLayer;
using antwika::editor::SheetStroke;
using antwika::map::kBaseLayer;
using antwika::map::Map;
using antwika::tile::TileRules;
using antwika::tilemap::Atlas;
using antwika::tilemap::Tile;

namespace
{
    constexpr Tile kOneTile{.atlas = Atlas::Wall, .index = 1};

    constexpr Tile kOtherTile{.atlas = Atlas::Wall, .index = 2};
}

TEST(LayerEditTest, IsDecorLayer_HoldsForEveryLayerButTheBase)
{
    EXPECT_FALSE(isDecorLayer(kBaseLayer));
    EXPECT_TRUE(isDecorLayer(kBaseLayer + 1));
    EXPECT_TRUE(isDecorLayer(kBaseLayer + 7));
}

TEST(LayerEditTest, ActiveRules_TakeTheMapsOwnRulesOnTheBaseLayer)
{
    Map drawnMap;

    EXPECT_EQ(&getActiveRules(drawnMap, kBaseLayer), &drawnMap.rules);
}

TEST(LayerEditTest, ActiveRules_TakeTheDecorRulesAboveTheBaseLayer)
{
    Map drawnMap;

    EXPECT_EQ(
        &getActiveRules(drawnMap, kBaseLayer + 1), &drawnMap.decorRules);
}

TEST(LayerEditTest, EditedTile_TakesThePickedTileOnTheBaseLayer)
{
    const Map drawnMap;
    SheetStroke stroke;
    AssignMode assignMode;

    stroke.selectedTile = kOneTile;
    assignMode.framePicked = 3;

    EXPECT_EQ(
        getEditedTile(drawnMap, kBaseLayer, stroke, assignMode), kOneTile);
}

TEST(LayerEditTest, EditedTile_TakesThePickedTileWithNoFrameChosen)
{
    const Map drawnMap;
    SheetStroke stroke;
    const AssignMode assignMode;

    stroke.selectedTile = kOneTile;

    EXPECT_EQ(
        getEditedTile(drawnMap, kBaseLayer + 1, stroke, assignMode),
        kOneTile);
}

TEST(LayerEditTest, EditedTile_FallsBackWhereTheDecorHoldsNoSuchFrame)
{
    const Map drawnMap;
    SheetStroke stroke;
    AssignMode assignMode;

    stroke.selectedTile = kOneTile;
    assignMode.framePicked = 9;

    EXPECT_EQ(
        getEditedTile(drawnMap, kBaseLayer + 1, stroke, assignMode),
        kOneTile);
}

TEST(LayerEditTest, IsAdjoining_HoldsWhereNoRuleSpeaksOfTheTiles)
{
    const TileRules rules;

    EXPECT_TRUE(isAdjoining(rules, kOneTile, kOtherTile));
}

TEST(LayerEditTest, IsAdjoining_LeavesTilesEveryEdgeShutsOut)
{
    TileRules rules;

    for (const auto edge : antwika::tilemap::kEveryTileEdge)
    {
        rules.forbidAll(kOneTile, edge);
        rules.forbidAll(kOtherTile, edge);
    }

    EXPECT_FALSE(isAdjoining(rules, kOneTile, kOtherTile));
}

TEST(LayerEditTest, IsAdjoining_HoldsWhereOneEdgeStillLetsThemMeet)
{
    TileRules rules;

    for (const auto edge : antwika::tilemap::kEveryTileEdge)
    {
        rules.forbidAll(kOneTile, edge);
        rules.forbidAll(kOtherTile, edge);
    }

    rules.allow(
        kOneTile, antwika::tilemap::kEveryTileEdge.front(), kOtherTile);
    rules.allow(
        kOtherTile,
        antwika::voxel::getFacing(antwika::tilemap::kEveryTileEdge.front()),
        kOneTile);

    EXPECT_TRUE(isAdjoining(rules, kOneTile, kOtherTile));
}
