#include <gtest/gtest.h>

#include <cstdint>

#include <antwika/geometry/Grid.hpp>
#include <antwika/tilemap/Slab.hpp>
#include <antwika/tilemap/TerrainClass.hpp>
#include <antwika/tilemap/TileMap.hpp>

#include "antwika/tilemap_demo/DemoMap.hpp"

using antwika::geometry::GridCell;
using antwika::tilemap::TerrainClass;
using antwika::tilemap_demo::demoMap;

namespace
{
    GridCell cellAt(const std::uint32_t column, const std::uint32_t row)
    {
        return GridCell{.column = column, .row = row};
    }
}

TEST(DemoMapTest, DemoMap_SpansTwentyByEleven)
{
    const auto map = demoMap();

    EXPECT_EQ(map.columns(), 20U);
    EXPECT_EQ(map.rows(), 11U);
}

TEST(DemoMapTest, DemoMap_CarriesTheDemoHeader)
{
    const auto map = demoMap();

    EXPECT_EQ(map.header().id, "wakewater-demo");
    EXPECT_EQ(map.header().ink.red, 214);
    EXPECT_EQ(map.header().paper.red, 12);
}

TEST(DemoMapTest, DemoMap_WallsEveryEdgeOfTheGrid)
{
    const auto map = demoMap();

    for (std::uint32_t column = 0; column < map.columns(); ++column)
    {
        EXPECT_EQ(
            map.at(cellAt(column, 0)).top()->terrain,
            TerrainClass::Wall);
        EXPECT_EQ(
            map.at(cellAt(column, 10)).top()->terrain,
            TerrainClass::Wall);
    }

    for (std::uint32_t row = 0; row < map.rows(); ++row)
    {
        EXPECT_EQ(
            map.at(cellAt(0, row)).top()->terrain, TerrainClass::Wall);
        EXPECT_EQ(
            map.at(cellAt(19, row)).top()->terrain,
            TerrainClass::Wall);
    }
}

TEST(DemoMapTest, DemoMap_LaysAPathAcrossTheMiddleRow)
{
    const auto map = demoMap();

    for (std::uint32_t column = 1; column <= 18; ++column)
    {
        if (column == 14)
        {
            continue;
        }

        EXPECT_EQ(
            map.at(cellAt(column, 5)).top()->terrain,
            TerrainClass::Path);
    }
}

TEST(DemoMapTest, DemoMap_PoolsWaterSouthOfThePath)
{
    const auto map = demoMap();

    EXPECT_EQ(
        map.at(cellAt(2, 6)).top()->terrain, TerrainClass::Water);
    EXPECT_EQ(
        map.at(cellAt(6, 9)).top()->terrain, TerrainClass::Water);
}

TEST(DemoMapTest, DemoMap_StacksTerraceSlabsUpToTheirHeight)
{
    const auto map = demoMap();
    const auto &terrace = map.at(cellAt(3, 2));

    EXPECT_EQ(terrace.slabs().size(), 2U);
    EXPECT_NE(terrace.slabAt(0), nullptr);
    EXPECT_NE(terrace.slabAt(1), nullptr);
    EXPECT_EQ(terrace.top()->terrain, TerrainClass::Floor);
}

TEST(DemoMapTest, DemoMap_RaisesTheEasternTerraceTwoLevels)
{
    const auto map = demoMap();
    const auto &terrace = map.at(cellAt(13, 1));

    EXPECT_EQ(terrace.slabs().size(), 3U);
    EXPECT_EQ(terrace.top()->level, 2);
}

TEST(DemoMapTest, DemoMap_PutsAStairBesideTheEasternTerrace)
{
    const auto map = demoMap();

    EXPECT_EQ(
        map.at(cellAt(14, 5)).top()->terrain, TerrainClass::Stair);
    EXPECT_EQ(map.at(cellAt(14, 5)).top()->level, 1);
}

TEST(DemoMapTest, DemoMap_RaisesTheCentralBlockFourLevelsOfWall)
{
    const auto map = demoMap();
    const auto &block = map.at(cellAt(7, 2));

    EXPECT_EQ(block.slabs().size(), 5U);
    EXPECT_EQ(block.top()->level, 4);
    EXPECT_EQ(block.top()->terrain, TerrainClass::Wall);
}
