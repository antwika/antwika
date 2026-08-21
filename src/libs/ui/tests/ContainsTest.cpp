#include <gtest/gtest.h>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>

#include "Contains.hpp"

using antwika::gfx::Point;
using antwika::gfx::Rect;
using antwika::ui::detail::contains;

namespace
{
    constexpr Rect kRect{.originPoint = {.x = 10, .y = 20},
                         .size = {.width = 4, .height = 6}};
}

TEST(ContainsTest, Contains_TakesAPointWithin)
{
    EXPECT_TRUE(contains(kRect, (Point{.x = 12, .y = 23})));
}

TEST(ContainsTest, Contains_TakesAPointOnTheLeftOrTopEdge)
{
    EXPECT_TRUE(contains(kRect, (Point{.x = 10, .y = 23})));
    EXPECT_TRUE(contains(kRect, (Point{.x = 12, .y = 20})));
}

TEST(ContainsTest, Contains_LeavesAPointOnTheRightOrBottomEdge)
{
    EXPECT_FALSE(contains(kRect, (Point{.x = 14, .y = 23})));
    EXPECT_FALSE(contains(kRect, (Point{.x = 12, .y = 26})));
}

TEST(ContainsTest, Contains_LetsNoTwoNeighboursTakeTheSamePoint)
{
    constexpr Rect rightRect{.originPoint = {.x = 14, .y = 20},
                         .size = {.width = 4, .height = 6}};
    constexpr Point sharedPoint{.x = 14, .y = 23};

    EXPECT_FALSE(contains(kRect, sharedPoint));
    EXPECT_TRUE(contains(rightRect, sharedPoint));
}
