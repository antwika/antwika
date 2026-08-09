#include <gtest/gtest.h>

#include "antwika/gfx/Point.hpp"

using antwika::gfx::Point;

namespace
{
    constexpr Point kReference{.x = 10, .y = 20};
}

TEST(PointTest, OperatorEquals_IsTrueForIdenticalCoordinates)
{
    constexpr Point same{.x = 10, .y = 20};

    EXPECT_EQ(kReference, same);
}

TEST(PointTest, OperatorEquals_IsFalseWhenTheXCoordinateDiffers)
{
    constexpr Point other{.x = 99, .y = 20};

    EXPECT_NE(kReference, other);
}

TEST(PointTest, OperatorEquals_IsFalseWhenTheYCoordinateDiffers)
{
    constexpr Point other{.x = 10, .y = 99};

    EXPECT_NE(kReference, other);
}
