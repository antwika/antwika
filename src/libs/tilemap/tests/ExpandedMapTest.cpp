#include <gtest/gtest.h>

#include <cstdint>
#include <variant>

#include <antwika/geometry/Grid.hpp>
#include <antwika/tilemap/Cell.hpp>
#include <antwika/tilemap/Entities.hpp>
#include <antwika/tilemap/ExpandedMap.hpp>
#include <antwika/tilemap/MapHeader.hpp>
#include <antwika/tilemap/Rgb.hpp>
#include <antwika/tilemap/TerrainClass.hpp>
#include <antwika/tilemap/TileMap.hpp>

using antwika::geometry::GridCell;
using antwika::tilemap::Cell;
using antwika::tilemap::expandedMap;
using antwika::tilemap::MapHeader;
using antwika::tilemap::TerrainClass;
using antwika::tilemap::TileMap;
using antwika::tilemap::TriggerVolume;

namespace
{
    [[nodiscard]] TileMap markedMap()
    {
        TileMap map(MapHeader{.id = "grown"}, 3, 2);

        map.at(GridCell{.column = 0, .row = 0}).terrain =
            TerrainClass::Water;
        map.at(GridCell{.column = 2, .row = 1}).terrain =
            TerrainClass::Cliff;

        return map;
    }
}

TEST(ExpandedMapTest, ExpandedMap_CopiesCornerCellsAtTheOffset)
{
    const auto grown = expandedMap(markedMap(), 2, 1, 1, 3);

    EXPECT_EQ(grown.columns(), 6U);
    EXPECT_EQ(grown.rows(), 6U);
    EXPECT_EQ(
        grown.at(GridCell{.column = 2, .row = 1}).terrain,
        TerrainClass::Water);
    EXPECT_EQ(
        grown.at(GridCell{.column = 4, .row = 2}).terrain,
        TerrainClass::Cliff);
    EXPECT_EQ(grown.at(GridCell{.column = 0, .row = 0}), Cell{});
    EXPECT_EQ(grown.at(GridCell{.column = 5, .row = 5}), Cell{});
}

TEST(ExpandedMapTest, ExpandedMap_ShiftsEntityOriginsByWestAndNorth)
{
    auto map = markedMap();

    map.addEntity(TriggerVolume{
        .id = "zone",
        .at = GridCell{.column = 1, .row = 1},
        .columns = 2,
        .rows = 1});

    const auto grown = expandedMap(map, 2, 1, 0, 0);
    const auto &trigger =
        std::get<TriggerVolume>(grown.entities().front());

    EXPECT_EQ(trigger.at, (GridCell{.column = 3, .row = 2}));
    EXPECT_EQ(trigger.columns, 2U);
    EXPECT_EQ(trigger.rows, 1U);
}

TEST(ExpandedMapTest, ExpandedMap_ReturnsAnEqualMapOnZeroGrowth)
{
    auto map = markedMap();

    map.addEntity(TriggerVolume{
        .id = "zone", .at = GridCell{.column = 1, .row = 0}});

    const auto grown = expandedMap(map, 0, 0, 0, 0);

    EXPECT_EQ(grown.columns(), map.columns());
    EXPECT_EQ(grown.rows(), map.rows());
    EXPECT_EQ(grown.entities(), map.entities());

    for (std::uint32_t row = 0; row < map.rows(); ++row)
    {
        for (std::uint32_t column = 0; column < map.columns();
             ++column)
        {
            const auto cell =
                GridCell{.column = column, .row = row};

            EXPECT_EQ(grown.at(cell), map.at(cell));
        }
    }
}

TEST(ExpandedMapTest, ExpandedMap_PreservesTheHeader)
{
    TileMap map(
        MapHeader{
            .id = "keeps-header",
            .ink = antwika::tilemap::Rgb{
                .red = 1, .green = 2, .blue = 3}},
        2,
        2);

    const auto grown = expandedMap(map, 1, 1, 1, 1);

    EXPECT_EQ(grown.header(), map.header());
}
