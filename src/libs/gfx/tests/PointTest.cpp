#include <gtest/gtest.h>

#include "antwika/gfx/Point.hpp"

using antwika::gfx::Point;

namespace
{
    constexpr Point kReferencePoint{.x = 10, .y = 20};
}

TEST(PointTest, OperatorEquals_IsTrueForIdenticalCoordinates)
{
    constexpr Point samePoint{.x = 10, .y = 20};

    EXPECT_EQ(kReferencePoint, samePoint);
}

TEST(PointTest, OperatorEquals_IsFalseWhenTheXCoordinateDiffers)
{
    constexpr Point otherPoint{.x = 99, .y = 20};

    EXPECT_NE(kReferencePoint, otherPoint);
}

TEST(PointTest, OperatorEquals_IsFalseWhenTheYCoordinateDiffers)
{
    constexpr Point otherPoint{.x = 10, .y = 99};

    EXPECT_NE(kReferencePoint, otherPoint);
}
