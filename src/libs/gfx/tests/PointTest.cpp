#include <gtest/gtest.h>

#include "antwika/gfx/Point.hpp"

using antwika::gfx::Point;

namespace
{
    constexpr Point kReference{.x = 10, .y = 20};
} // namespace

TEST(PointTest, Equality_IsTrueForIdenticalCoordinates)
{
    constexpr Point same{.x = 10, .y = 20};

    EXPECT_EQ(kReference, same);
}

TEST(PointTest, Equality_IsFalseWhenTheXCoordinateDiffers)
{
    constexpr Point other{.x = 99, .y = 20};

    EXPECT_NE(kReference, other);
}

TEST(PointTest, Equality_IsFalseWhenTheYCoordinateDiffers)
{
    constexpr Point other{.x = 10, .y = 99};

    EXPECT_NE(kReference, other);
}
