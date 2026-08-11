#include <gtest/gtest.h>

#include <cstdint>
#include <limits>

#include <antwika/tilemap/Column.hpp>
#include <antwika/tilemap/Slab.hpp>
#include <antwika/tilemap/TerrainClass.hpp>

using antwika::tilemap::Column;
using antwika::tilemap::Slab;
using antwika::tilemap::TerrainClass;

TEST(ColumnTest, Column_DefaultsToEmpty)
{
    const Column column{};

    EXPECT_TRUE(column.slabs().empty());
}

TEST(ColumnTest, Place_KeepsSlabsSortedAscending)
{
    Column column;

    (void)column.place(Slab{.level = 2});
    (void)column.place(Slab{.level = -1});
    (void)column.place(Slab{.level = 0});

    ASSERT_EQ(column.slabs().size(), 3U);
    EXPECT_EQ(column.slabs()[0].level, -1);
    EXPECT_EQ(column.slabs()[1].level, 0);
    EXPECT_EQ(column.slabs()[2].level, 2);
    EXPECT_EQ(column.top()->level, 2);
}

TEST(ColumnTest, Place_ReplacesTheSlabAtAnExistingLevel)
{
    Column column;

    (void)column.place(
        Slab{.level = 1, .terrain = TerrainClass::Floor});
    auto &placed = column.place(
        Slab{.level = 1, .terrain = TerrainClass::Water});

    ASSERT_EQ(column.slabs().size(), 1U);
    EXPECT_EQ(placed.terrain, TerrainClass::Water);
    EXPECT_EQ(column.slabAt(1)->terrain, TerrainClass::Water);
}

TEST(ColumnTest, SlabAt_FindsOnlyAnExactLevel)
{
    Column column;

    (void)column.place(Slab{.level = 1});
    (void)column.place(Slab{.level = 3});

    EXPECT_EQ(column.slabAt(2), nullptr);
    EXPECT_EQ(column.slabAt(4), nullptr);
    ASSERT_NE(column.slabAt(3), nullptr);
    EXPECT_EQ(column.slabAt(3)->level, 3);

    const auto &read = column;
    EXPECT_EQ(read.slabAt(0), nullptr);
    ASSERT_NE(read.slabAt(1), nullptr);
    EXPECT_EQ(read.slabAt(1)->level, 1);
}

TEST(ColumnTest, Top_IsNullOnAnEmptyColumn)
{
    Column column;

    EXPECT_EQ(column.top(), nullptr);

    const auto &read = column;
    EXPECT_EQ(read.top(), nullptr);
}

TEST(ColumnTest, TopAtOrBelow_SkipsSlabsAboveTheLevel)
{
    Column column;

    (void)column.place(Slab{.level = 0});
    (void)column.place(Slab{.level = 5});

    ASSERT_NE(column.topAtOrBelow(4), nullptr);
    EXPECT_EQ(column.topAtOrBelow(4)->level, 0);
    ASSERT_NE(column.topAtOrBelow(5), nullptr);
    EXPECT_EQ(column.topAtOrBelow(5)->level, 5);
    ASSERT_NE(column.topAtOrBelow(7), nullptr);
    EXPECT_EQ(column.topAtOrBelow(7)->level, 5);
    EXPECT_EQ(column.topAtOrBelow(-1), nullptr);
}

TEST(ColumnTest, Standable_RequiresOneEmptyLevelAbove)
{
    constexpr auto kTopLevel =
        std::numeric_limits<std::int32_t>::max();

    Column column;

    (void)column.place(Slab{.level = 0});
    EXPECT_TRUE(column.standable(0));

    (void)column.place(Slab{.level = 2});
    EXPECT_TRUE(column.standable(0));

    (void)column.place(Slab{.level = 1});
    EXPECT_FALSE(column.standable(0));

    (void)column.place(Slab{.level = kTopLevel});
    EXPECT_TRUE(column.standable(kTopLevel));
}

TEST(ColumnTest, Standable_IsFalseWithoutASlab)
{
    Column column;

    EXPECT_FALSE(column.standable(0));

    (void)column.place(Slab{.level = 1});
    EXPECT_FALSE(column.standable(0));
}

TEST(ColumnTest, Remove_TellsWhetherALevelHeldASlab)
{
    Column column;

    (void)column.place(Slab{.level = 1});

    EXPECT_FALSE(column.remove(2));
    EXPECT_TRUE(column.remove(1));
    EXPECT_TRUE(column.slabs().empty());
    EXPECT_FALSE(column.remove(1));
}

TEST(ColumnTest, Remove_KeepsALevelBelowTheLowestSlab)
{
    Column column;

    (void)column.place(Slab{.level = 5});

    EXPECT_FALSE(column.remove(3));
    EXPECT_EQ(column.slabs().size(), 1U);
}

TEST(ColumnTest, Clear_EmptiesTheColumn)
{
    Column column;

    (void)column.place(Slab{.level = 0});
    (void)column.place(Slab{.level = 4});

    column.clear();

    EXPECT_TRUE(column.slabs().empty());
}

TEST(ColumnTest, OperatorEquals_ComparesEverySlab)
{
    Column base;
    (void)base.place(Slab{.level = 0});
    (void)base.place(Slab{.level = 2, .light = 7});

    Column twin;
    (void)twin.place(Slab{.level = 0});
    (void)twin.place(Slab{.level = 2, .light = 7});

    EXPECT_EQ(base, twin);

    auto other = twin;
    other.slabAt(2)->light = 8;
    EXPECT_NE(base, other);

    other = twin;
    (void)other.place(Slab{.level = 5});
    EXPECT_NE(base, other);
}
