#include <optional>

#include <gtest/gtest.h>

#include <antwika/gfx/Size.hpp>

#include "antwika/atlas_editor/Pixel.hpp"
#include "antwika/atlas_editor/Selection.hpp"

using antwika::atlas_editor::clampedTo;
using antwika::atlas_editor::contains;
using antwika::atlas_editor::movedBy;
using antwika::atlas_editor::Pixel;
using antwika::atlas_editor::Selection;
using antwika::atlas_editor::selectionBetween;
using antwika::gfx::Size;

namespace
{
    constexpr Size kSheet{.width = 10, .height = 8};
} // namespace

// Both corners are in it, so two corners two apart span three pixels.
// Off by one here is a column of art nobody meant to move.
TEST(SelectionTest, Between_HoldsBothCornersItWasGiven)
{
    const auto marked =
        selectionBetween(Pixel{.x = 2, .y = 3}, Pixel{.x = 4, .y = 6});

    EXPECT_EQ(marked.origin, (Pixel{.x = 2, .y = 3}));
    EXPECT_EQ(marked.size, (Size{.width = 3, .height = 4}));
}

// A drag up and to the left selects what it crossed.
// Which is the same rectangle as the drag back the other way.
TEST(SelectionTest, Between_ReadsTheSameDragEitherWayRound)
{
    const auto forth =
        selectionBetween(Pixel{.x = 2, .y = 3}, Pixel{.x = 4, .y = 6});
    const auto back =
        selectionBetween(Pixel{.x = 4, .y = 6}, Pixel{.x = 2, .y = 3});

    EXPECT_EQ(forth, back);
}

TEST(SelectionTest, Between_SelectsOnePixelWhenTheDragWentNowhere)
{
    const auto marked =
        selectionBetween(Pixel{.x = 5, .y = 5}, Pixel{.x = 5, .y = 5});

    EXPECT_EQ(marked.size, (Size{.width = 1, .height = 1}));
    EXPECT_TRUE(contains(marked, Pixel{.x = 5, .y = 5}));
}

TEST(SelectionTest, Contains_TakesTheEdgesAndNotWhatIsPastThem)
{
    constexpr Selection marked{
        .origin = {.x = 2, .y = 3}, .size = {.width = 3, .height = 4}};

    EXPECT_TRUE(contains(marked, Pixel{.x = 2, .y = 3}));
    EXPECT_TRUE(contains(marked, Pixel{.x = 4, .y = 6}));
    EXPECT_FALSE(contains(marked, Pixel{.x = 1, .y = 3}));
    EXPECT_FALSE(contains(marked, Pixel{.x = 2, .y = 2}));
    EXPECT_FALSE(contains(marked, Pixel{.x = 5, .y = 6}));
    EXPECT_FALSE(contains(marked, Pixel{.x = 4, .y = 7}));
}

TEST(SelectionTest, MovedBy_SlidesItWithoutResizingIt)
{
    constexpr Selection marked{
        .origin = {.x = 2, .y = 3}, .size = {.width = 3, .height = 4}};

    const auto moved = movedBy(marked, -4, 2);

    EXPECT_EQ(moved.origin, (Pixel{.x = -2, .y = 5}));
    EXPECT_EQ(moved.size, marked.size);
}

TEST(SelectionTest, ClampedTo_LeavesOneAlreadyInsideAlone)
{
    constexpr Selection marked{
        .origin = {.x = 2, .y = 3}, .size = {.width = 3, .height = 4}};

    EXPECT_EQ(clampedTo(marked, kSheet), marked);
}

TEST(SelectionTest, ClampedTo_CutsOffWhatHangsOverAnEdge)
{
    constexpr Selection over{
        .origin = {.x = -2, .y = -1}, .size = {.width = 5, .height = 4}};

    const auto cut = clampedTo(over, kSheet);

    ASSERT_TRUE(cut.has_value());
    EXPECT_EQ(cut->origin, (Pixel{.x = 0, .y = 0}));
    EXPECT_EQ(cut->size, (Size{.width = 3, .height = 3}));
}

TEST(SelectionTest, ClampedTo_CutsOffWhatHangsPastTheFarCorner)
{
    constexpr Selection over{
        .origin = {.x = 8, .y = 6}, .size = {.width = 6, .height = 6}};

    const auto cut = clampedTo(over, kSheet);

    ASSERT_TRUE(cut.has_value());
    EXPECT_EQ(cut->origin, (Pixel{.x = 8, .y = 6}));
    EXPECT_EQ(cut->size, (Size{.width = 2, .height = 2}));
}

// Nothing overlapping is nothing to act on.
// Rather than a rectangle of no extent every caller would have to test.
TEST(SelectionTest, ClampedTo_AnswersNothingForOneEntirelyOutside)
{
    constexpr Selection past{
        .origin = {.x = 20, .y = 2}, .size = {.width = 3, .height = 3}};
    constexpr Selection before{
        .origin = {.x = -9, .y = 2}, .size = {.width = 3, .height = 3}};
    constexpr Selection below{
        .origin = {.x = 2, .y = 40}, .size = {.width = 3, .height = 3}};
    constexpr Selection above{
        .origin = {.x = 2, .y = -8}, .size = {.width = 3, .height = 3}};

    EXPECT_FALSE(clampedTo(past, kSheet).has_value());
    EXPECT_FALSE(clampedTo(before, kSheet).has_value());
    EXPECT_FALSE(clampedTo(below, kSheet).has_value());
    EXPECT_FALSE(clampedTo(above, kSheet).has_value());
}

TEST(SelectionTest, TwoSelectionsCompareByOriginAndSize)
{
    constexpr Selection marked{
        .origin = {.x = 2, .y = 3}, .size = {.width = 3, .height = 4}};
    constexpr Selection elsewhere{
        .origin = {.x = 5, .y = 3}, .size = {.width = 3, .height = 4}};
    constexpr Selection bigger{
        .origin = {.x = 2, .y = 3}, .size = {.width = 9, .height = 4}};

    EXPECT_EQ(marked, marked);
    EXPECT_NE(marked, elsewhere);
    EXPECT_NE(marked, bigger);
}
