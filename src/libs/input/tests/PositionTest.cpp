#include <gtest/gtest.h>

#include "antwika/input/Position.hpp"

using antwika::input::Position;

namespace
{
    constexpr Position kReference{.x = 10, .y = 20};
} // namespace

TEST(PositionTest, DefaultConstruction_IsTheOrigin)
{
    constexpr Position origin;

    EXPECT_EQ(origin.x, 0);
    EXPECT_EQ(origin.y, 0);
}

TEST(PositionTest, Equality_IsTrueForIdenticalCoordinates)
{
    constexpr Position same{.x = 10, .y = 20};

    EXPECT_EQ(kReference, same);
}

TEST(PositionTest, Equality_IsFalseWhenTheXCoordinateDiffers)
{
    constexpr Position other{.x = 99, .y = 20};

    EXPECT_NE(kReference, other);
}

TEST(PositionTest, Equality_IsFalseWhenTheYCoordinateDiffers)
{
    constexpr Position other{.x = 10, .y = 99};

    EXPECT_NE(kReference, other);
}

TEST(PositionTest, Coordinates_MayFallOutsideTheSurface)
{
    // Signed on purpose.
    // A drag past the left or top edge reports a negative coordinate.
    constexpr Position outside{.x = -5, .y = -12};

    EXPECT_EQ(outside.x, -5);
    EXPECT_EQ(outside.y, -12);
}
