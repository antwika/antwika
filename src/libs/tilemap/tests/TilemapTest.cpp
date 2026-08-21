#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <set>
#include <utility>

#include <antwika/geometry/Grid.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/tilemap/AtlasLayout.hpp>
#include <antwika/tilemap/Tilemap.hpp>

using antwika::tilemap::Atlas;
using antwika::tilemap::atlasSize;
using antwika::tilemap::cellHoldingTile;
using antwika::tilemap::clearTile;
using antwika::tilemap::defaultTilemap;
using antwika::tilemap::gridCellSize;
using antwika::tilemap::kAtlasColumns;
using antwika::tilemap::kAtlasRows;
using antwika::tilemap::kFloorTileSize;
using antwika::tilemap::kWallTileSize;
using antwika::tilemap::putTile;
using antwika::tilemap::suggestedTileFor;
using antwika::tilemap::swapTiles;
using antwika::tilemap::Tile;
using antwika::tilemap::Tilemap;
using antwika::tilemap::tileSizeOf;
using antwika::tilemap::tileSource;

namespace
{
    constexpr antwika::gfx::Size kCanvasSize{
        .width = 320, .height = 180};

    constexpr float kTolerance = 0.001F;

    [[nodiscard]] float right(const antwika::gfx::RectF &placeRect)
    {
        return placeRect.originPoint.x + placeRect.size.width;
    }

    [[nodiscard]] float bottom(const antwika::gfx::RectF &placeRect)
    {
        return placeRect.originPoint.y + placeRect.size.height;
    }
}

TEST(TilemapTest, DefaultTilemap_HoldsEveryTileOfBothAtlases)
{
    const auto map = defaultTilemap();

    EXPECT_EQ(map.columns, 2U * kAtlasColumns);
    EXPECT_EQ(map.rows, static_cast<std::uint32_t>(kAtlasRows));
    EXPECT_TRUE(map.isComplete());

    std::set<std::pair<int, std::uint16_t>> seenPairs;

    for (const auto tile : map.tiles)
    {
        ASSERT_TRUE(tile.has_value());
        seenPairs.insert(
            {static_cast<int>(tile->atlas), tile->index});
    }

    EXPECT_EQ(seenPairs.size(), map.tiles.size());
    EXPECT_EQ(seenPairs.size(), 2U * kAtlasColumns * kAtlasRows);
}

TEST(TilemapTest, DefaultTilemap_PutsTheWallAtlasOnTheLeft)
{
    const auto map = defaultTilemap();
    const auto columnCount = static_cast<std::uint32_t>(kAtlasColumns);

    for (std::uint32_t row = 0; row < map.rows; ++row)
    {
        for (std::uint32_t column = 0; column < map.columns;
             ++column)
        {
            const auto tile = map.at(column, row);

            ASSERT_TRUE(tile.has_value());
            EXPECT_EQ(
                tile->atlas,
                column < columnCount ? Atlas::Wall : Atlas::Floor)
                << column << row;
        }
    }
}

TEST(TilemapTest, DefaultTilemap_LaysEachAtlasOutAsItsOwnAtlasIs)
{
    const auto map = defaultTilemap();
    const auto columnCount = static_cast<std::uint32_t>(kAtlasColumns);

    EXPECT_EQ(map.at(0, 0), (Tile{.atlas = Atlas::Wall, .index = 0}));
    EXPECT_EQ(
        map.at(columnCount, 0), (Tile{.atlas = Atlas::Floor, .index = 0}));
    EXPECT_EQ(
        map.at(1, 0), (Tile{.atlas = Atlas::Wall, .index = 1}));
    EXPECT_EQ(
        map.at(0, 1),
        (Tile{
            .atlas = Atlas::Wall,
            .index = static_cast<std::uint16_t>(kAtlasColumns)}));
    EXPECT_EQ(
        map.at(map.columns - 1, map.rows - 1),
        (Tile{
            .atlas = Atlas::Floor,
            .index = static_cast<std::uint16_t>(
                kAtlasColumns * kAtlasRows - 1)}));
}

TEST(TilemapTest, TileSizeOf_TellsTheTwoAtlasesApart)
{
    EXPECT_EQ(tileSizeOf(Atlas::Wall), kWallTileSize);
    EXPECT_EQ(tileSizeOf(Atlas::Floor), kFloorTileSize);
}

TEST(TilemapTest, TileSource_TakesEveryTileFromInsideItsAtlas)
{
    for (const auto slot : defaultTilemap().tiles)
    {
        ASSERT_TRUE(slot.has_value());

        const auto tile = *slot;
        const auto wholeSize = atlasSize(tileSizeOf(tile.atlas));
        const auto source = tileSource(tile);

        EXPECT_GE(source.originPoint.x, 0.0F);
        EXPECT_GE(source.originPoint.y, 0.0F);
        EXPECT_LE(right(source), static_cast<float>(wholeSize.width));
        EXPECT_LE(bottom(source), static_cast<float>(wholeSize.height));
        EXPECT_FLOAT_EQ(
            source.size.width,
            static_cast<float>(tileSizeOf(tile.atlas).width));
    }
}

TEST(TilemapTest, TilemapCell_HoldsTheLargerOfTheTwoTiles)
{
    const auto cell = gridCellSize();

    EXPECT_GE(cell.width, kWallTileSize.width);
    EXPECT_GE(cell.width, kFloorTileSize.width);
    EXPECT_GE(cell.height, kWallTileSize.height);
    EXPECT_GE(cell.height, kFloorTileSize.height);
}

TEST(TilemapTest, SwapTiles_PutsEachTileWhereTheOtherWas)
{
    auto map = defaultTilemap();

    const antwika::geometry::GridCell fromCell{.column = 0, .row = 0};
    const antwika::geometry::GridCell toCell{.column = 20, .row = 7};

    const auto was = map.at(fromCell.column, fromCell.row);
    const auto toTile = map.at(toCell.column, toCell.row);

    ASSERT_NE(was, toTile);

    swapTiles(map, fromCell, toCell);

    EXPECT_EQ(map.at(fromCell.column, fromCell.row), toTile);
    EXPECT_EQ(map.at(toCell.column, toCell.row), was);
}

TEST(TilemapTest, SwapTiles_LeavesEveryOtherPlaceAlone)
{
    const auto beforeTilemap = defaultTilemap();
    auto afterTilemap = beforeTilemap;

    const antwika::geometry::GridCell fromCell{.column = 3, .row = 2};
    const antwika::geometry::GridCell toCell{.column = 25, .row = 11};

    swapTiles(afterTilemap, fromCell, toCell);

    for (std::uint32_t row = 0; row < beforeTilemap.rows; ++row)
    {
        for (std::uint32_t column = 0; column < beforeTilemap.columns;
             ++column)
        {
            const antwika::geometry::GridCell placeCell{
                .column = column, .row = row};

            if (placeCell == fromCell || placeCell == toCell)
            {
                continue;
            }

            EXPECT_EQ(
                afterTilemap.at(column, row), beforeTilemap.at(column, row))
                << column << row;
        }
    }
}

TEST(TilemapTest, SwapTiles_ComesBackWhenDoneTwice)
{
    const auto beforeTilemap = defaultTilemap();
    auto afterTilemap = beforeTilemap;

    const antwika::geometry::GridCell fromCell{.column = 1, .row = 1};
    const antwika::geometry::GridCell toCell{.column = 30, .row = 14};

    swapTiles(afterTilemap, fromCell, toCell);
    swapTiles(afterTilemap, fromCell, toCell);

    EXPECT_EQ(afterTilemap, beforeTilemap);
}

TEST(TilemapTest, SwapTiles_LeavesAPlaceSwappedWithItselfAlone)
{
    const auto beforeTilemap = defaultTilemap();
    auto afterTilemap = beforeTilemap;

    const antwika::geometry::GridCell placeCell{.column = 4, .row = 4};

    swapTiles(afterTilemap, placeCell, placeCell);

    EXPECT_EQ(afterTilemap, beforeTilemap);
}

TEST(TilemapTest, SwapTiles_MovesATileBetweenTheTwoAtlases)
{
    auto map = defaultTilemap();

    const antwika::geometry::GridCell uprightCell{.column = 0, .row = 0};
    const antwika::geometry::GridCell flatCell{
        .column = static_cast<std::uint32_t>(kAtlasColumns),
        .row = 0};

    ASSERT_EQ(map.at(uprightCell.column, uprightCell.row)->atlas,
              Atlas::Wall);
    ASSERT_EQ(map.at(flatCell.column, flatCell.row)->atlas, Atlas::Floor);

    swapTiles(map, uprightCell, flatCell);

    EXPECT_EQ(
        map.at(uprightCell.column, uprightCell.row)->atlas, Atlas::Floor);
    EXPECT_EQ(map.at(flatCell.column, flatCell.row)->atlas, Atlas::Wall);
}

TEST(TilemapTest, CellHoldingTile_FindsWhereAGridHoldsATile)
{
    const auto map = defaultTilemap();
    const auto tile = map.at(9, 6);
    ASSERT_TRUE(tile.has_value());

    const auto sits = cellHoldingTile(map, *tile);

    ASSERT_TRUE(sits.has_value());
    EXPECT_EQ(sits->column, 9U);
    EXPECT_EQ(sits->row, 6U);
}

TEST(TilemapTest, CellHoldingTile_FindsATileWhereverASwapHasPutIt)
{
    auto map = defaultTilemap();
    const auto tile = map.at(9, 6);

    swapTiles(map, {.column = 9, .row = 6}, {.column = 1, .row = 0});

    ASSERT_TRUE(tile.has_value());

    const auto sits = cellHoldingTile(map, *tile);

    ASSERT_TRUE(sits.has_value());
    EXPECT_EQ(sits->column, 1U);
    EXPECT_EQ(sits->row, 0U);
}

TEST(TilemapTest, CellHoldingTile_FindsNothingForATileNoPlaceHolds)
{
    const auto map = defaultTilemap();

    EXPECT_FALSE(
        cellHoldingTile(map, Tile{.atlas = Atlas::Floor, .index = 9999})
            .has_value());
}

TEST(TilemapTest, ClearTile_TakesTheTileFromOnePlaceOnly)
{
    auto map = defaultTilemap();
    const auto tile = map.at(5, 3);

    clearTile(map, {.column = 5, .row = 3});

    EXPECT_FALSE(map.at(5, 3).has_value());
    EXPECT_TRUE(map.at(4, 3).has_value());
    EXPECT_TRUE(map.at(6, 3).has_value());
    EXPECT_TRUE(map.isComplete());
    ASSERT_TRUE(tile.has_value());
    EXPECT_FALSE(cellHoldingTile(map, *tile).has_value());
}

TEST(TilemapTest, ClearTile_LeavesThePlaceThereToBeSwappedInto)
{
    auto map = defaultTilemap();
    const auto secondTile = map.at(1, 0);

    clearTile(map, {.column = 0, .row = 0});
    swapTiles(map, {.column = 0, .row = 0}, {.column = 1, .row = 0});

    EXPECT_EQ(map.at(0, 0), secondTile);
    EXPECT_FALSE(map.at(1, 0).has_value());
}

TEST(TilemapTest, SuggestedTileFor_GivesAPlaceBackWhatBelongsToIt)
{
    auto map = defaultTilemap();
    const auto was = map.at(5, 3);

    clearTile(map, {.column = 5, .row = 3});

    EXPECT_EQ(suggestedTileFor(map, {.column = 5, .row = 3}), was);
}

TEST(TilemapTest, SuggestedTileFor_GivesSomethingTheGridLacksInstead)
{
    auto map = defaultTilemap();
    const auto was = map.at(5, 3);

    clearTile(map, {.column = 5, .row = 3});
    clearTile(map, {.column = 6, .row = 3});
    putTile(map, {.column = 6, .row = 3}, *was);

    const auto suggestion =
        suggestedTileFor(map, {.column = 5, .row = 3});

    ASSERT_TRUE(suggestion.has_value());
    EXPECT_NE(suggestion, was);
    EXPECT_FALSE(cellHoldingTile(map, *suggestion).has_value());
}

TEST(TilemapTest, SuggestedTileFor_KeepsToTheAtlasThePlaceBelongsTo)
{
    auto map = defaultTilemap();

    for (const auto column : {0U, 20U})
    {
        const auto was = map.at(column, 2);

        clearTile(map, {.column = column, .row = 2});

        const auto suggestion =
            suggestedTileFor(map, {.column = column, .row = 2});

        ASSERT_TRUE(suggestion.has_value());
        ASSERT_TRUE(was.has_value());
        EXPECT_EQ(suggestion->atlas, was->atlas);
    }
}

TEST(TilemapTest, SuggestedTileFor_GivesNothingWhereTheGridHoldsThemAll)
{
    const auto map = defaultTilemap();

    EXPECT_FALSE(
        suggestedTileFor(map, {.column = 5, .row = 3}).has_value());
}

TEST(TilemapTest, PutTile_FillsThePlaceAndNoOther)
{
    auto map = defaultTilemap();
    const auto secondTile = map.at(6, 3);

    clearTile(map, {.column = 5, .row = 3});
    putTile(
        map,
        {.column = 5, .row = 3},
        Tile{.atlas = Atlas::Floor, .index = 99});

    EXPECT_EQ(map.at(5, 3)->index, 99U);
    EXPECT_EQ(map.at(6, 3), secondTile);
}

TEST(TilemapTest, PutTile_AndClearComeBackToWhereItWas)
{
    const auto was = defaultTilemap();
    auto map = was;
    const auto tile = map.at(4, 1);

    clearTile(map, {.column = 4, .row = 1});
    putTile(map, {.column = 4, .row = 1}, *tile);

    EXPECT_EQ(map, was);
}
