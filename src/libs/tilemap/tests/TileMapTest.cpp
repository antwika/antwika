#include <gtest/gtest.h>

#include <cstdint>
#include <variant>

#include <antwika/geometry/Grid.hpp>
#include <antwika/tilemap/Column.hpp>
#include <antwika/tilemap/Entities.hpp>
#include <antwika/tilemap/MapHeader.hpp>
#include <antwika/tilemap/Slab.hpp>
#include <antwika/tilemap/TerrainClass.hpp>
#include <antwika/tilemap/TileMap.hpp>
#include <antwika/tilemap/TileMapError.hpp>

using antwika::geometry::GridCell;
using antwika::tilemap::Column;
using antwika::tilemap::MapHeader;
using antwika::tilemap::Npc;
using antwika::tilemap::Slab;
using antwika::tilemap::TerrainClass;
using antwika::tilemap::TileMap;
using antwika::tilemap::TileMapError;
using antwika::tilemap::Transition;

namespace
{
    TileMap mapOf(const std::uint32_t columns, const std::uint32_t rows)
    {
        return TileMap(MapHeader{.id = "test"}, columns, rows);
    }
}

TEST(TileMapTest, Ctor_ThrowsOnZeroColumns)
{
    EXPECT_THROW(mapOf(0, 2), TileMapError);
}

TEST(TileMapTest, Ctor_ThrowsOnZeroRows)
{
    EXPECT_THROW(mapOf(3, 0), TileMapError);
}

TEST(TileMapTest, Ctor_FillsEveryColumnWithADefaultSlabAtLevelZero)
{
    const auto map = mapOf(3, 2);

    Column single;
    (void)single.place(Slab{});

    EXPECT_EQ(map.columns(), 3U);
    EXPECT_EQ(map.rows(), 2U);
    EXPECT_EQ(map.at(GridCell{.column = 0, .row = 0}), single);
    EXPECT_EQ(map.at(GridCell{.column = 2, .row = 1}), single);
}

TEST(TileMapTest, Ctor_KeepsTheHeaderItWasGiven)
{
    const TileMap map(MapHeader{.id = "wakewater-01"}, 1, 1);

    EXPECT_EQ(map.header().id, "wakewater-01");
}

TEST(TileMapTest, At_AddressesColumnsByColumnAndRow)
{
    auto map = mapOf(3, 2);

    (void)map.at(GridCell{.column = 2, .row = 0})
        .place(Slab{.level = 1, .terrain = TerrainClass::Water});
    (void)map.at(GridCell{.column = 0, .row = 1})
        .place(Slab{.level = -1, .terrain = TerrainClass::Cliff});

    const auto &read = map;
    ASSERT_NE(
        read.at(GridCell{.column = 2, .row = 0}).slabAt(1), nullptr);
    EXPECT_EQ(
        read.at(GridCell{.column = 2, .row = 0}).slabAt(1)->terrain,
        TerrainClass::Water);
    ASSERT_NE(
        read.at(GridCell{.column = 0, .row = 1}).slabAt(-1),
        nullptr);
    EXPECT_EQ(
        read.at(GridCell{.column = 0, .row = 1}).slabAt(-1)->terrain,
        TerrainClass::Cliff);
    EXPECT_EQ(
        read.at(GridCell{.column = 1, .row = 0}).slabs().size(), 1U);
    EXPECT_EQ(
        read.at(GridCell{.column = 1, .row = 1}).slabs().size(), 1U);
}

TEST(TileMapTest, At_ThrowsOnAColumnBeyondTheGrid)
{
    auto map = mapOf(3, 2);

    EXPECT_THROW(
        (void)map.at(GridCell{.column = 3, .row = 0}), TileMapError);
}

TEST(TileMapTest, At_ThrowsOnARowBeyondTheGrid)
{
    auto map = mapOf(3, 2);

    EXPECT_THROW(
        (void)map.at(GridCell{.column = 0, .row = 2}), TileMapError);
}

TEST(TileMapTest, Entities_StartEmpty)
{
    const auto map = mapOf(1, 1);

    EXPECT_TRUE(map.entities().empty());
}

TEST(TileMapTest, AddEntity_AppendsInOrder)
{
    auto map = mapOf(1, 1);

    map.addEntity(Transition{.id = "door-east"});
    map.addEntity(Npc{.id = "keeper"});

    ASSERT_EQ(map.entities().size(), 2U);
    EXPECT_EQ(std::get<Transition>(map.entities()[0]).id, "door-east");
    EXPECT_EQ(std::get<Npc>(map.entities()[1]).id, "keeper");
}
