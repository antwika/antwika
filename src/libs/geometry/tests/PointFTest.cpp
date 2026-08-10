#include <gtest/gtest.h>

#include <antwika/geometry/Point.hpp>
#include <antwika/geometry/PointF.hpp>

using antwika::geometry::Point;
using antwika::geometry::PointF;

TEST(PointFTest, Default_StartsAtTheOrigin)
{
    constexpr PointF point;

    EXPECT_FLOAT_EQ(point.x, 0.0F);
    EXPECT_FLOAT_EQ(point.y, 0.0F);
}

TEST(PointFTest, Coordinates_AreKeptAsGiven)
{
    constexpr PointF point{1.5F, -2.25F};

    EXPECT_FLOAT_EQ(point.x, 1.5F);
    EXPECT_FLOAT_EQ(point.y, -2.25F);
}

TEST(PointFTest, AnIntegerPoint_WidensWithoutLosingItsCoordinates)
{
    constexpr PointF point = Point{.x = -3, .y = 7};

    EXPECT_FLOAT_EQ(point.x, -3.0F);
    EXPECT_FLOAT_EQ(point.y, 7.0F);
}

TEST(PointFTest, Equality_ComparesBothCoordinates)
{
    constexpr PointF point{1.0F, 2.0F};

    EXPECT_EQ(point, (PointF{1.0F, 2.0F}));
    EXPECT_NE(point, (PointF{1.0F, 3.0F}));
    EXPECT_NE(point, (PointF{3.0F, 2.0F}));
}
