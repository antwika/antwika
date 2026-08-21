#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>

#include <antwika/geometry/Grid.hpp>
#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/tilemap/AtlasLayout.hpp>
#include <antwika/tilemap/Tilemap.hpp>

#include "antwika/editor/ui/EditorLook.hpp"
#include "antwika/editor/ui/TilemapView.hpp"

using antwika::editor::cellAtPoint;
using antwika::editor::kPaneMargin;
using antwika::editor::tilePlace;
using antwika::editor::tilemapPlace;
using antwika::tilemap::Atlas;
using antwika::tilemap::kAtlasColumns;
using antwika::tilemap::kAtlasRows;
using antwika::tilemap::tileSizeOf;
using antwika::tilemap::defaultTilemap;
using antwika::tilemap::gridCellSize;
using antwika::tilemap::kFloorTileSize;
using antwika::tilemap::kWallTileSize;
using antwika::tilemap::Tile;
using antwika::tilemap::Tilemap;

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

TEST(TilemapViewTest, TilemapPlace_KeepsTheGridOnTheCanvas)
{
    const auto where = tilemapPlace(kCanvasSize, defaultTilemap());

    EXPECT_GE(where.originPoint.x, kPaneMargin - kTolerance);
    EXPECT_GE(where.originPoint.y, kPaneMargin - kTolerance);
    EXPECT_LE(
        right(where),
        static_cast<float>(kCanvasSize.width) - kPaneMargin
            + kTolerance);
    EXPECT_LE(
        bottom(where),
        static_cast<float>(kCanvasSize.height) - kPaneMargin
            + kTolerance);
}

TEST(TilemapViewTest, TilemapPlace_KeepsTheShapeThePlacesGiveIt)
{
    const auto map = defaultTilemap();
    const auto cell = gridCellSize();
    const auto where = tilemapPlace(kCanvasSize, map);

    EXPECT_NEAR(
        where.size.width / where.size.height,
        static_cast<float>(map.columns * cell.width)
            / static_cast<float>(map.rows * cell.height),
        0.01F);
}

TEST(TilemapViewTest, TilePlace_KeepsEveryTileInsideTheGrid)
{
    const auto map = defaultTilemap();
    const auto where = tilemapPlace(kCanvasSize, map);

    for (std::uint32_t row = 0; row < map.rows; ++row)
    {
        for (std::uint32_t column = 0; column < map.columns;
             ++column)
        {
            const auto place = tilePlace(map, column, row, where);

            EXPECT_GE(place.originPoint.x, where.originPoint.x - kTolerance);
            EXPECT_GE(place.originPoint.y, where.originPoint.y - kTolerance);
            EXPECT_LE(right(place), right(where) + kTolerance);
            EXPECT_LE(bottom(place), bottom(where) + kTolerance);
        }
    }
}

TEST(TilemapViewTest, TilePlace_LeavesOnePlaceClearOfTheNext)
{
    const auto map = defaultTilemap();
    const auto where = tilemapPlace(kCanvasSize, map);

    const auto first = tilePlace(map, 0, 0, where);
    const auto besideRect = tilePlace(map, 1, 0, where);
    const auto belowRect = tilePlace(map, 0, 1, where);

    EXPECT_LE(right(first), besideRect.originPoint.x + kTolerance);
    EXPECT_LE(bottom(first), belowRect.originPoint.y + kTolerance);
}

TEST(TilemapViewTest, TilePlace_KeepsATileTheShapeItIs)
{
    const auto map = defaultTilemap();
    const auto where = tilemapPlace(kCanvasSize, map);
    const auto columnCount = static_cast<std::uint32_t>(kAtlasColumns);

    for (const auto column : {0U, columnCount})
    {
        const auto tile = map.at(column, 0);
        const auto place = tilePlace(map, column, 0, where);

        ASSERT_TRUE(tile.has_value());

        const auto size = tileSizeOf(tile->atlas);

        EXPECT_NEAR(
            place.size.width / place.size.height,
            static_cast<float>(size.width)
                / static_cast<float>(size.height),
            0.01F)
            << column;
    }
}

TEST(TilemapViewTest, TilePlace_DrawsTheShorterTilesShorter)
{
    const auto map = defaultTilemap();
    const auto where = tilemapPlace(kCanvasSize, map);
    const auto columnCount = static_cast<std::uint32_t>(kAtlasColumns);

    EXPECT_LT(
        tilePlace(map, 0, 0, where).size.height,
        tilePlace(map, columnCount, 0, where).size.height);
}

TEST(TilemapViewTest, CellAtPoint_FindsThePlaceAPointFallsOn)
{
    const auto map = defaultTilemap();
    const auto where = tilemapPlace(kCanvasSize, map);

    for (const auto wantedCell :
         {antwika::geometry::GridCell{.column = 0, .row = 0},
          antwika::geometry::GridCell{.column = 5, .row = 9},
          antwika::geometry::GridCell{
              .column = map.columns - 1, .row = map.rows - 1}})
    {
        const auto place = tilePlace(
            map, wantedCell.column, wantedCell.row, where);
        const antwika::gfx::PointF middlePoint{
            place.originPoint.x + (place.size.width / 2.0F),
            place.originPoint.y + (place.size.height / 2.0F)};

        const auto foundCell = cellAtPoint(map, where, middlePoint);

        ASSERT_TRUE(foundCell.has_value());
        EXPECT_EQ(*foundCell, wantedCell);
    }
}

TEST(TilemapViewTest, CellAtPoint_FindsNothingBeyondTheGrid)
{
    const auto map = defaultTilemap();
    const auto where = tilemapPlace(kCanvasSize, map);

    const antwika::gfx::PointF abovePoint{
        where.originPoint.x + 1.0F, where.originPoint.y - 1.0F};
    const antwika::gfx::PointF leftPoint{
        where.originPoint.x - 1.0F, where.originPoint.y + 1.0F};
    const antwika::gfx::PointF beyondPoint{
        right(where) + 1.0F, bottom(where) - 1.0F};
    const antwika::gfx::PointF belowPoint{
        right(where) - 1.0F, bottom(where) + 1.0F};

    EXPECT_FALSE(cellAtPoint(map, where, abovePoint).has_value());
    EXPECT_FALSE(cellAtPoint(map, where, leftPoint).has_value());
    EXPECT_FALSE(cellAtPoint(map, where, beyondPoint).has_value());
    EXPECT_FALSE(cellAtPoint(map, where, belowPoint).has_value());
}

TEST(TilemapViewTest, CellAtPoint_FindsNothingInAGridWithNoPlaces)
{
    const Tilemap bareTilemap;

    EXPECT_FALSE(
        cellAtPoint(
            bareTilemap,
            tilemapPlace(kCanvasSize, bareTilemap),
            {0.0F, 0.0F})
            .has_value());
}
