#include <gtest/gtest.h>

#include <antwika/geometry/PointF.hpp>
#include <antwika/geometry/Rect.hpp>
#include <antwika/geometry/RectF.hpp>
#include <antwika/geometry/SizeF.hpp>

using antwika::geometry::PointF;
using antwika::geometry::Rect;
using antwika::geometry::RectF;
using antwika::geometry::SizeF;

TEST(RectFTest, Default_IsEmptyAtTheOrigin)
{
    constexpr RectF rect;

    EXPECT_EQ(rect.origin, PointF{});
    EXPECT_EQ(rect.size, SizeF{});
}

TEST(RectFTest, OriginAndSize_AreKeptAsGiven)
{
    constexpr RectF rect{PointF{1.5F, 2.5F}, SizeF{3.5F, 4.5F}};

    EXPECT_EQ(rect.origin, (PointF{1.5F, 2.5F}));
    EXPECT_EQ(rect.size, (SizeF{3.5F, 4.5F}));
}

TEST(RectFTest, AnIntegerRect_WidensWithoutLosingItsBounds)
{
    constexpr RectF rect = Rect{
        .origin = {.x = -2, .y = 3}, .size = {.width = 8, .height = 6}};

    EXPECT_EQ(rect.origin, (PointF{-2.0F, 3.0F}));
    EXPECT_EQ(rect.size, (SizeF{8.0F, 6.0F}));
}

TEST(RectFTest, Equality_ComparesOriginAndSize)
{
    constexpr RectF rect{PointF{1.0F, 2.0F}, SizeF{3.0F, 4.0F}};

    EXPECT_EQ(rect, (RectF{PointF{1.0F, 2.0F}, SizeF{3.0F, 4.0F}}));
    EXPECT_NE(rect, (RectF{PointF{9.0F, 2.0F}, SizeF{3.0F, 4.0F}}));
    EXPECT_NE(rect, (RectF{PointF{1.0F, 2.0F}, SizeF{3.0F, 9.0F}}));
}
