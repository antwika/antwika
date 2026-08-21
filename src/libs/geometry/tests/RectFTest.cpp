#include <gtest/gtest.h>

#include <antwika/geometry/PointF.hpp>
#include <antwika/geometry/Rect.hpp>
#include <antwika/geometry/RectF.hpp>
#include <antwika/geometry/SizeF.hpp>

using antwika::geometry::holds;
using antwika::geometry::PointF;
using antwika::geometry::Rect;
using antwika::geometry::RectF;
using antwika::geometry::SizeF;

TEST(RectFTest, Default_IsEmptyAtTheOrigin)
{
    constexpr RectF rect;

    EXPECT_EQ(rect.originPoint, PointF{});
    EXPECT_EQ(rect.size, SizeF{});
}

TEST(RectFTest, OriginAndSize_AreKeptAsGiven)
{
    constexpr RectF rect{PointF{1.5F, 2.5F}, SizeF{3.5F, 4.5F}};

    EXPECT_EQ(rect.originPoint, (PointF{1.5F, 2.5F}));
    EXPECT_EQ(rect.size, (SizeF{3.5F, 4.5F}));
}

TEST(RectFTest, AnIntegerRect_WidensWithoutLosingItsBounds)
{
    constexpr RectF rect = Rect{
        .originPoint = {.x = -2, .y = 3}, .size = {.width = 8, .height = 6}};

    EXPECT_EQ(rect.originPoint, (PointF{-2.0F, 3.0F}));
    EXPECT_EQ(rect.size, (SizeF{8.0F, 6.0F}));
}

TEST(RectFTest, Equality_ComparesOriginAndSize)
{
    constexpr RectF rect{PointF{1.0F, 2.0F}, SizeF{3.0F, 4.0F}};

    EXPECT_EQ(rect, (RectF{PointF{1.0F, 2.0F}, SizeF{3.0F, 4.0F}}));
    EXPECT_NE(rect, (RectF{PointF{9.0F, 2.0F}, SizeF{3.0F, 4.0F}}));
    EXPECT_NE(rect, (RectF{PointF{1.0F, 2.0F}, SizeF{3.0F, 9.0F}}));
}

TEST(RectFTest, Holds_TakesAPointWithin)
{
    constexpr RectF rect{PointF{1.0F, 2.0F}, SizeF{4.0F, 6.0F}};

    EXPECT_TRUE(holds(rect, (PointF{3.0F, 5.0F})));
}

TEST(RectFTest, Holds_TakesAPointOnEveryEdge)
{
    constexpr RectF rect{PointF{1.0F, 2.0F}, SizeF{4.0F, 6.0F}};

    EXPECT_TRUE(holds(rect, (PointF{1.0F, 5.0F})));
    EXPECT_TRUE(holds(rect, (PointF{5.0F, 5.0F})));
    EXPECT_TRUE(holds(rect, (PointF{3.0F, 2.0F})));
    EXPECT_TRUE(holds(rect, (PointF{3.0F, 8.0F})));
}

TEST(RectFTest, Holds_TakesNothingBeyondAnySide)
{
    constexpr RectF rect{PointF{1.0F, 2.0F}, SizeF{4.0F, 6.0F}};

    EXPECT_FALSE(holds(rect, (PointF{0.5F, 5.0F})));
    EXPECT_FALSE(holds(rect, (PointF{5.5F, 5.0F})));
    EXPECT_FALSE(holds(rect, (PointF{3.0F, 1.5F})));
    EXPECT_FALSE(holds(rect, (PointF{3.0F, 8.5F})));
}

TEST(RectFTest, Holds_TakesOnlyItsOwnCornerWhenEmpty)
{
    constexpr RectF rect{PointF{1.0F, 2.0F}, SizeF{}};

    EXPECT_TRUE(holds(rect, (PointF{1.0F, 2.0F})));
    EXPECT_FALSE(holds(rect, (PointF{1.5F, 2.0F})));
}
