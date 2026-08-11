#include <gtest/gtest.h>

#include <cstdint>
#include <variant>

#include <antwika/geometry/Grid.hpp>
#include <antwika/tilemap/Column.hpp>
#include <antwika/tilemap/Entities.hpp>
#include <antwika/tilemap/ExpandedMap.hpp>
#include <antwika/tilemap/MapHeader.hpp>
#include <antwika/tilemap/Rgb.hpp>
#include <antwika/tilemap/Slab.hpp>
#include <antwika/tilemap/TerrainClass.hpp>
#include <antwika/tilemap/TileMap.hpp>

using antwika::geometry::GridCell;
using antwika::tilemap::BoatEmbark;
using antwika::tilemap::Column;
using antwika::tilemap::expandedMap;
using antwika::tilemap::MapHeader;
using antwika::tilemap::Npc;
using antwika::tilemap::Pickup;
using antwika::tilemap::Slab;
using antwika::tilemap::SpawnPoint;
using antwika::tilemap::TerrainClass;
using antwika::tilemap::TileMap;
using antwika::tilemap::Transition;
using antwika::tilemap::TriggerVolume;

namespace
{
    [[nodiscard]] TileMap markedMap()
    {
        TileMap map(MapHeader{.id = "grown"}, 3, 2);

        map.at(GridCell{.column = 0, .row = 0}).top()->terrain =
            TerrainClass::Water;
        (void)map.at(GridCell{.column = 2, .row = 1})
            .place(Slab{.level = 3, .terrain = TerrainClass::Cliff});

        return map;
    }
}

TEST(ExpandedMapTest, ExpandedMap_CopiesCornerColumnsAtTheOffset)
{
    const auto grown = expandedMap(markedMap(), 2, 1, 1, 3);

    EXPECT_EQ(grown.columns(), 6U);
    EXPECT_EQ(grown.rows(), 6U);
    EXPECT_EQ(
        grown.at(GridCell{.column = 2, .row = 1}).top()->terrain,
        TerrainClass::Water);
    ASSERT_NE(
        grown.at(GridCell{.column = 4, .row = 2}).slabAt(3),
        nullptr);
    EXPECT_EQ(
        grown.at(GridCell{.column = 4, .row = 2}).slabAt(3)->terrain,
        TerrainClass::Cliff);

    Column single;
    (void)single.place(Slab{});

    EXPECT_EQ(grown.at(GridCell{.column = 0, .row = 0}), single);
    EXPECT_EQ(grown.at(GridCell{.column = 5, .row = 5}), single);
}

TEST(ExpandedMapTest, ExpandedMap_ShiftsEntityOriginsByWestAndNorth)
{
    auto map = markedMap();

    map.addEntity(TriggerVolume{
        .id = "zone",
        .at = GridCell{.column = 1, .row = 1},
        .level = -2,
        .columns = 2,
        .rows = 1});

    const auto grown = expandedMap(map, 2, 1, 0, 0);
    const auto &trigger =
        std::get<TriggerVolume>(grown.entities().front());

    EXPECT_EQ(trigger.at, (GridCell{.column = 3, .row = 2}));
    EXPECT_EQ(trigger.level, -2);
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

TEST(ExpandedMapTest, ExpandedMap_ShiftsTheOriginOfEveryEntityKind)
{
    auto map = markedMap();
    const auto origin = GridCell{.column = 1, .row = 1};

    map.addEntity(Transition{.id = "door", .at = origin});
    map.addEntity(BoatEmbark{.id = "jetty", .at = origin});
    map.addEntity(SpawnPoint{.id = "den", .at = origin});
    map.addEntity(Pickup{.id = "chest", .at = origin});
    map.addEntity(Npc{.id = "ferryman", .at = origin});
    map.addEntity(TriggerVolume{.id = "zone", .at = origin});

    const auto grown = expandedMap(map, 2, 3, 0, 0);

    ASSERT_EQ(grown.entities().size(), 6U);

    for (const auto &entity : grown.entities())
    {
        std::visit(
            [](const auto &kind)
            {
                EXPECT_EQ(
                    kind.at, (GridCell{.column = 3, .row = 4}));
            },
            entity);
    }
}
