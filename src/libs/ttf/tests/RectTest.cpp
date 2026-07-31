#include <gtest/gtest.h>

#include "antwika/ttf/Rect.hpp"

using antwika::ttf::Rect;

namespace
{
    constexpr Rect kReference{
        .x = 10, .y = 20, .width = 30, .height = 40};
} // namespace

TEST(RectTest, Equality_IsTrueForTheSameFourNumbers)
{
    constexpr Rect same{.x = 10, .y = 20, .width = 30, .height = 40};

    EXPECT_EQ(kReference, same);
}

TEST(RectTest, Equality_IsFalseWhenTheColumnDiffers)
{
    constexpr Rect other{.x = 99, .y = 20, .width = 30, .height = 40};

    EXPECT_NE(kReference, other);
}

TEST(RectTest, Equality_IsFalseWhenTheRowDiffers)
{
    constexpr Rect other{.x = 10, .y = 99, .width = 30, .height = 40};

    EXPECT_NE(kReference, other);
}

TEST(RectTest, Equality_IsFalseWhenTheWidthDiffers)
{
    constexpr Rect other{.x = 10, .y = 20, .width = 99, .height = 40};

    EXPECT_NE(kReference, other);
}

TEST(RectTest, Equality_IsFalseWhenTheHeightDiffers)
{
    constexpr Rect other{.x = 10, .y = 20, .width = 30, .height = 99};

    EXPECT_NE(kReference, other);
}
