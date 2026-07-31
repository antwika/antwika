#include <cstdint>

#include <gtest/gtest.h>

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

    TEST(GridLayoutTest, CellsAreSquareAndTheGridSitsBelowTheBar)
    {
        const auto layout = layoutFor(kCanvas, 12, 8);
        ASSERT_TRUE(layout.has_value());
        EXPECT_EQ(layout->cell, 80U);
        EXPECT_EQ(layout->origin.x, 0);

        // 720 - 48 leaves 672; eight rows of 80 leave 32 to share.
        EXPECT_EQ(
            layout->origin.y,
            static_cast<std::int32_t>(scoreBarHeight(kCanvas) + 16));
    }

    TEST(GridLayoutTest, AnEmptyGridHasNoLayout)
    {
        EXPECT_FALSE(layoutFor(kCanvas, 0, 8).has_value());
        EXPECT_FALSE(layoutFor(kCanvas, 12, 0).has_value());
    }

    TEST(GridLayoutTest, ACanvasWithNoRoomHasNoLayout)
    {
        // A canvas the score bar alone fills leaves no rows at all.
        constexpr Size kShort{.width = 960, .height = 16};
        ASSERT_EQ(scoreBarHeight(kShort), kShort.height);
        EXPECT_FALSE(layoutFor(kShort, 12, 8).has_value());

        // Room below the bar, but not a whole pixel for every column.
        EXPECT_FALSE(
            layoutFor({.width = 4, .height = 20}, 12, 8).has_value());
    }

    // The window the app opens, whose numbers every other test uses.
    TEST(GridLayoutTest, TheAppsOwnCanvasReservesFortyEightPixels)
    {
        EXPECT_EQ(scoreBarHeight(kCanvas), 48U);
    }

    TEST(GridLayoutTest, APointFindsTheCellItLandsIn)
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

    TEST(GridLayoutTest, AClickOnTheScoreBarFindsNoCell)
    {
        const auto layout = layoutFor(kCanvas, 12, 8);
        ASSERT_TRUE(layout.has_value());

        // The whole reason the bar's strip is taken off the top.
        EXPECT_FALSE(cellAt(*layout, 100, 4).has_value());
        EXPECT_FALSE(cellAt(*layout, -1, layout->origin.y).has_value());
    }

    TEST(GridLayoutTest, APointPastTheGridFindsNoCell)
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

    TEST(GridLayoutTest, ACellsRectangleIsWhereAClickInItLands)
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

    TEST(GridLayoutTest, TwoLayoutsOfTheSameThingAreEqual)
    {
        EXPECT_EQ(layoutFor(kCanvas, 12, 8), layoutFor(kCanvas, 12, 8));
        EXPECT_NE(layoutFor(kCanvas, 12, 8), layoutFor(kCanvas, 10, 8));
    }

    // A layout two scenes disagree about is two different boards.
    // So every field has to count towards a layout being the same.
    // A defaulted operator== stops at the first difference found.
    // So each field is asserted on its own.
    TEST(GridLayoutTest, EveryFieldOfALayoutCountsTowardsEquality)
    {
        constexpr GridLayout base{
            .width = 12,
            .height = 8,
            .cell = 80,
            .origin = {.x = 4, .y = 64}};

        EXPECT_EQ(base, base);

        GridLayout other = base;
        other.width = 11;
        EXPECT_NE(base, other);

        other = base;
        other.height = 7;
        EXPECT_NE(base, other);

        other = base;
        other.cell = 79;
        EXPECT_NE(base, other);

        other = base;
        other.origin = {.x = 5, .y = 64};
        EXPECT_NE(base, other);
    }
} // namespace
