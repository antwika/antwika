#include <gtest/gtest.h>

#include "antwika/input/Position.hpp"

using antwika::input::Position;

namespace
{
    constexpr Position kReferencePosition{.x = 10, .y = 20};
}

TEST(PositionTest, Ctor_IsTheOrigin)
{
    constexpr Position originPointPosition;

    EXPECT_EQ(originPointPosition.x, 0);
    EXPECT_EQ(originPointPosition.y, 0);
}

TEST(PositionTest, OperatorEquals_IsTrueForIdenticalCoordinates)
{
    constexpr Position samePosition{.x = 10, .y = 20};

    EXPECT_EQ(kReferencePosition, samePosition);
}

TEST(PositionTest, OperatorEquals_IsFalseWhenTheXCoordinateDiffers)
{
    constexpr Position otherPosition{.x = 99, .y = 20};

    EXPECT_NE(kReferencePosition, otherPosition);
}

TEST(PositionTest, OperatorEquals_IsFalseWhenTheYCoordinateDiffers)
{
    constexpr Position otherPosition{.x = 10, .y = 99};

    EXPECT_NE(kReferencePosition, otherPosition);
}

TEST(PositionTest, Coordinates_MayFallOutsideTheSurface)
{
    constexpr Position outsidePosition{.x = -5, .y = -12};

    EXPECT_EQ(outsidePosition.x, -5);
    EXPECT_EQ(outsidePosition.y, -12);
}
