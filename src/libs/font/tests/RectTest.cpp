#include <gtest/gtest.h>

#include "antwika/font/Rect.hpp"

using antwika::font::Rect;

namespace
{
    constexpr Rect kReferenceRect{
        .x = 10, .y = 20, .width = 30, .height = 40};
}

TEST(RectTest, OperatorEquals_IsTrueForTheSameFourNumbers)
{
    constexpr Rect sameRect{.x = 10, .y = 20, .width = 30, .height = 40};

    EXPECT_EQ(kReferenceRect, sameRect);
}

TEST(RectTest, OperatorEquals_IsFalseWhenTheColumnDiffers)
{
    constexpr Rect otherRect{.x = 99, .y = 20, .width = 30, .height = 40};

    EXPECT_NE(kReferenceRect, otherRect);
}

TEST(RectTest, OperatorEquals_IsFalseWhenTheRowDiffers)
{
    constexpr Rect otherRect{.x = 10, .y = 99, .width = 30, .height = 40};

    EXPECT_NE(kReferenceRect, otherRect);
}

TEST(RectTest, OperatorEquals_IsFalseWhenTheWidthDiffers)
{
    constexpr Rect otherRect{.x = 10, .y = 20, .width = 99, .height = 40};

    EXPECT_NE(kReferenceRect, otherRect);
}

TEST(RectTest, OperatorEquals_IsFalseWhenTheHeightDiffers)
{
    constexpr Rect otherRect{.x = 10, .y = 20, .width = 30, .height = 99};

    EXPECT_NE(kReferenceRect, otherRect);
}
