#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include <antwika/autotile/Cutaway.hpp>
#include <antwika/geometry/Grid.hpp>
#include <antwika/tilemap/MapHeader.hpp>
#include <antwika/tilemap/Slab.hpp>
#include <antwika/tilemap/TileMap.hpp>

using antwika::autotile::cutawayHidden;
using antwika::geometry::GridCell;
using antwika::tilemap::MapHeader;
using antwika::tilemap::Slab;
using antwika::tilemap::TileMap;

namespace
{
    GridCell cellAt(const std::uint32_t column, const std::uint32_t row)
    {
        return GridCell{.column = column, .row = row};
    }

    TileMap mapOf(const std::uint32_t columns, const std::uint32_t rows)
    {
        return TileMap(MapHeader{.id = "cutaway"}, columns, rows);
    }

    void raise(
        TileMap &map, const GridCell cell, const std::int32_t level)
    {
        (void)map.at(cell).place(Slab{.level = level});
    }

    bool hiddenAt(
        const std::vector<bool> &flags,
        const TileMap &map,
        const GridCell cell)
    {
        return flags.at(
            static_cast<std::size_t>(cell.row) * map.columns()
            + cell.column);
    }
}

TEST(CutawayTest, CutawayHidden_ReturnsOneFlagPerCellRowByRow)
{
    const auto map = mapOf(3, 2);

    const auto hidden = cutawayHidden(map, cellAt(0, 0), 0);

    EXPECT_EQ(hidden.size(), 6U);
}

TEST(CutawayTest, CutawayHidden_LeavesEveryColumnVisibleWhenNoneRises)
{
    const auto map = mapOf(3, 3);

    const auto hidden = cutawayHidden(map, cellAt(1, 1), 0);

    for (const auto flag : hidden)
    {
        EXPECT_FALSE(flag);
    }
}

TEST(CutawayTest, CutawayHidden_LeavesAColumnLevelWithThePlayerVisible)
{
    auto map = mapOf(3, 3);
    raise(map, cellAt(1, 1), 2);

    const auto hidden = cutawayHidden(map, cellAt(1, 1), 2);

    EXPECT_FALSE(hiddenAt(hidden, map, cellAt(1, 1)));
}

TEST(CutawayTest, CutawayHidden_HidesThePlayersOwnColumnWhenItRises)
{
    auto map = mapOf(3, 3);
    raise(map, cellAt(1, 1), 2);

    const auto hidden = cutawayHidden(map, cellAt(1, 1), 0);

    EXPECT_TRUE(hiddenAt(hidden, map, cellAt(1, 1)));
}

TEST(CutawayTest, CutawayHidden_LeavesAnEmptyColumnVisible)
{
    auto map = mapOf(3, 3);
    map.at(cellAt(1, 1)).clear();

    const auto hidden = cutawayHidden(map, cellAt(1, 1), -5);

    EXPECT_FALSE(hiddenAt(hidden, map, cellAt(1, 1)));
}

TEST(CutawayTest, CutawayHidden_SeedsTheTwoRowsInFrontOfThePlayer)
{
    auto map = mapOf(3, 4);
    raise(map, cellAt(1, 1), 2);
    raise(map, cellAt(1, 2), 2);

    const auto hidden = cutawayHidden(map, cellAt(1, 0), 0);

    EXPECT_TRUE(hiddenAt(hidden, map, cellAt(1, 1)));
    EXPECT_TRUE(hiddenAt(hidden, map, cellAt(1, 2)));
}

TEST(CutawayTest, CutawayHidden_LeavesTheThirdRowInFrontUnseeded)
{
    auto map = mapOf(3, 4);
    raise(map, cellAt(1, 3), 2);

    const auto hidden = cutawayHidden(map, cellAt(1, 0), 0);

    EXPECT_FALSE(hiddenAt(hidden, map, cellAt(1, 3)));
}

TEST(CutawayTest, CutawayHidden_SeedsTheColumnsFlankingTheRowsInFront)
{
    auto map = mapOf(3, 3);
    raise(map, cellAt(0, 1), 2);
    raise(map, cellAt(2, 1), 2);

    const auto hidden = cutawayHidden(map, cellAt(1, 0), 0);

    EXPECT_TRUE(hiddenAt(hidden, map, cellAt(0, 1)));
    EXPECT_TRUE(hiddenAt(hidden, map, cellAt(2, 1)));
}

TEST(CutawayTest, CutawayHidden_SkipsAFlankLeftOfTheGridsFirstColumn)
{
    auto map = mapOf(2, 2);
    raise(map, cellAt(0, 1), 2);

    const auto hidden = cutawayHidden(map, cellAt(0, 0), 0);

    EXPECT_TRUE(hiddenAt(hidden, map, cellAt(0, 1)));
}

TEST(CutawayTest, CutawayHidden_SkipsAFlankRightOfTheGridsLastColumn)
{
    auto map = mapOf(2, 2);
    raise(map, cellAt(1, 1), 2);

    const auto hidden = cutawayHidden(map, cellAt(1, 0), 0);

    EXPECT_TRUE(hiddenAt(hidden, map, cellAt(1, 1)));
}

TEST(CutawayTest, CutawayHidden_SkipsSeedRowsPastTheGridsLastRow)
{
    auto map = mapOf(2, 2);
    raise(map, cellAt(1, 1), 2);

    const auto hidden = cutawayHidden(map, cellAt(1, 1), 0);

    EXPECT_EQ(hidden.size(), 4U);
    EXPECT_TRUE(hiddenAt(hidden, map, cellAt(1, 1)));
}

TEST(CutawayTest, CutawayHidden_FloodsEastAndSouthFromASeededColumn)
{
    auto map = mapOf(3, 3);
    raise(map, cellAt(1, 1), 2);
    raise(map, cellAt(2, 1), 2);
    raise(map, cellAt(2, 2), 2);

    const auto hidden = cutawayHidden(map, cellAt(1, 1), 0);

    EXPECT_TRUE(hiddenAt(hidden, map, cellAt(2, 1)));
    EXPECT_TRUE(hiddenAt(hidden, map, cellAt(2, 2)));
}

TEST(CutawayTest, CutawayHidden_FloodsWestAndNorthFromASeededColumn)
{
    auto map = mapOf(3, 3);
    raise(map, cellAt(1, 1), 2);
    raise(map, cellAt(0, 1), 2);
    raise(map, cellAt(1, 0), 2);

    const auto hidden = cutawayHidden(map, cellAt(1, 1), 0);

    EXPECT_TRUE(hiddenAt(hidden, map, cellAt(0, 1)));
    EXPECT_TRUE(hiddenAt(hidden, map, cellAt(1, 0)));
}

TEST(CutawayTest, CutawayHidden_StopsTheFloodAtALowColumn)
{
    auto map = mapOf(3, 1);
    raise(map, cellAt(0, 0), 2);
    raise(map, cellAt(2, 0), 2);

    const auto hidden = cutawayHidden(map, cellAt(0, 0), 0);

    EXPECT_TRUE(hiddenAt(hidden, map, cellAt(0, 0)));
    EXPECT_FALSE(hiddenAt(hidden, map, cellAt(1, 0)));
    EXPECT_FALSE(hiddenAt(hidden, map, cellAt(2, 0)));
}

TEST(CutawayTest, CutawayHidden_VisitsACellReachableTwiceOnlyOnce)
{
    auto map = mapOf(2, 2);

    for (std::uint32_t column = 0; column < 2; ++column)
    {
        for (std::uint32_t row = 0; row < 2; ++row)
        {
            raise(map, cellAt(column, row), 2);
        }
    }

    const auto hidden = cutawayHidden(map, cellAt(0, 0), 0);

    for (const auto flag : hidden)
    {
        EXPECT_TRUE(flag);
    }
}
