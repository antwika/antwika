#include <gtest/gtest.h>

#include "antwika/font/Rect.hpp"

using antwika::font::Rect;

namespace
{
    constexpr Rect kReference{
        .x = 10, .y = 20, .width = 30, .height = 40};
}

TEST(RectTest, OperatorEquals_IsTrueForTheSameFourNumbers)
{
    constexpr Rect same{.x = 10, .y = 20, .width = 30, .height = 40};

    EXPECT_EQ(kReference, same);
}

TEST(RectTest, OperatorEquals_IsFalseWhenTheColumnDiffers)
{
    constexpr Rect other{.x = 99, .y = 20, .width = 30, .height = 40};

    EXPECT_NE(kReference, other);
}

TEST(RectTest, OperatorEquals_IsFalseWhenTheRowDiffers)
{
    constexpr Rect other{.x = 10, .y = 99, .width = 30, .height = 40};

    EXPECT_NE(kReference, other);
}

TEST(RectTest, OperatorEquals_IsFalseWhenTheWidthDiffers)
{
    constexpr Rect other{.x = 10, .y = 20, .width = 99, .height = 40};

    EXPECT_NE(kReference, other);
}

TEST(RectTest, OperatorEquals_IsFalseWhenTheHeightDiffers)
{
    constexpr Rect other{.x = 10, .y = 20, .width = 30, .height = 99};

    EXPECT_NE(kReference, other);
}
