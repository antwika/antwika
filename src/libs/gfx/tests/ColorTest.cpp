#include <gtest/gtest.h>

#include "antwika/gfx/Color.hpp"

using antwika::gfx::Color;

namespace
{
    constexpr Color kReference{
        .red = 1,
        .green = 2,
        .blue = 3,
        .alpha = 4};
} // namespace

TEST(ColorTest, Equality_IsTrueForIdenticalChannels)
{
    constexpr Color same{.red = 1, .green = 2, .blue = 3, .alpha = 4};

    EXPECT_EQ(kReference, same);
}

TEST(ColorTest, Equality_IsFalseWhenTheRedChannelDiffers)
{
    constexpr Color other{.red = 9, .green = 2, .blue = 3, .alpha = 4};

    EXPECT_NE(kReference, other);
}

TEST(ColorTest, Equality_IsFalseWhenTheGreenChannelDiffers)
{
    constexpr Color other{.red = 1, .green = 9, .blue = 3, .alpha = 4};

    EXPECT_NE(kReference, other);
}

TEST(ColorTest, Equality_IsFalseWhenTheBlueChannelDiffers)
{
    constexpr Color other{.red = 1, .green = 2, .blue = 9, .alpha = 4};

    EXPECT_NE(kReference, other);
}

TEST(ColorTest, Equality_IsFalseWhenTheAlphaChannelDiffers)
{
    constexpr Color other{.red = 1, .green = 2, .blue = 3, .alpha = 9};

    EXPECT_NE(kReference, other);
}
