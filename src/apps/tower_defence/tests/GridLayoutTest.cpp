#include <gtest/gtest.h>

#include <cstdint>

#include "antwika/tower_defence/GridLayout.hpp"

using antwika::gfx::Size;
using antwika::tower_defence::Cell;
using antwika::tower_defence::cellAt;
using antwika::tower_defence::cellRect;
using antwika::tower_defence::GridLayout;
using antwika::tower_defence::scoreBarHeight;
using antwika::tower_defence::layoutFor;

namespace
{
    constexpr Size kCanvas{.width = 960, .height = 720};

    TEST(GridLayoutTest, LayoutFor_MakesSquareCellsBelowTheBar)
    {
        const auto layout = layoutFor(kCanvas, 12, 8);
        ASSERT_TRUE(layout.has_value());
        EXPECT_EQ(layout->cell, 80U);
        EXPECT_EQ(layout->origin.x, 0);

        EXPECT_EQ(
            layout->origin.y,
            static_cast<std::int32_t>(scoreBarHeight(kCanvas) + 16));
    }

    TEST(GridLayoutTest, LayoutFor_GivesNoneForAnEmptyGrid)
    {
        EXPECT_FALSE(layoutFor(kCanvas, 0, 8).has_value());
        EXPECT_FALSE(layoutFor(kCanvas, 12, 0).has_value());
    }

    TEST(GridLayoutTest, LayoutFor_GivesNoneForACanvasWithNoRoom)
    {
        constexpr Size kShort{.width = 960, .height = 16};
        ASSERT_EQ(scoreBarHeight(kShort), kShort.height);
        EXPECT_FALSE(layoutFor(kShort, 12, 8).has_value());

        EXPECT_FALSE(
            layoutFor({.width = 4, .height = 20}, 12, 8).has_value());
    }

    TEST(GridLayoutTest, ScoreBarHeight_ReservesFortyEightPixels)
    {
        EXPECT_EQ(scoreBarHeight(kCanvas), 48U);
    }

    TEST(GridLayoutTest, CellAt_FindsTheCellAPointLandsIn)
    {
        const auto layout = layoutFor(kCanvas, 12, 8);
        ASSERT_TRUE(layout.has_value());

        const auto first = cellAt(*layout, layout->origin.x, layout->origin.y);
        ASSERT_TRUE(first.has_value());
        EXPECT_EQ(*first, (Cell{.x = 0, .y = 0}));

        const auto middle = cellAt(
            *layout,
            layout->origin.x + 200,
            layout->origin.y + 170);
        ASSERT_TRUE(middle.has_value());
        EXPECT_EQ(*middle, (Cell{.x = 2, .y = 2}));
    }

    TEST(GridLayoutTest, CellAt_FindsNoCellOnTheScoreBar)
    {
        const auto layout = layoutFor(kCanvas, 12, 8);
        ASSERT_TRUE(layout.has_value());

        EXPECT_FALSE(cellAt(*layout, 100, 4).has_value());
        EXPECT_FALSE(cellAt(*layout, -1, layout->origin.y).has_value());
    }

    TEST(GridLayoutTest, CellAt_FindsNoCellPastTheGrid)
    {
        const auto layout = layoutFor(kCanvas, 12, 8);
        ASSERT_TRUE(layout.has_value());
        EXPECT_FALSE(
            cellAt(*layout, layout->origin.x + 12 * 80, layout->origin.y)
                .has_value());
        EXPECT_FALSE(
            cellAt(*layout, layout->origin.x, layout->origin.y + 8 * 80)
                .has_value());
    }

    TEST(GridLayoutTest, CellRect_MatchesWhereAClickLands)
    {
        const auto layout = layoutFor(kCanvas, 12, 8);
        ASSERT_TRUE(layout.has_value());

        const Cell cell{.x = 5, .y = 3};
        const auto rect = cellRect(*layout, cell);
        EXPECT_EQ(rect.size.width, layout->cell);
        EXPECT_EQ(rect.size.height, layout->cell);

        const auto found = cellAt(*layout, rect.origin.x, rect.origin.y);
        ASSERT_TRUE(found.has_value());
        EXPECT_EQ(*found, cell);
    }

    TEST(GridLayoutTest, OperatorEquals_MatchesTwoLayoutsOfOneThing)
    {
        EXPECT_EQ(layoutFor(kCanvas, 12, 8), layoutFor(kCanvas, 12, 8));
        EXPECT_NE(layoutFor(kCanvas, 12, 8), layoutFor(kCanvas, 10, 8));
    }
}
