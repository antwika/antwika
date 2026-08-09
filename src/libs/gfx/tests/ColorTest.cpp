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
}

TEST(ColorTest, OperatorEquals_IsTrueForIdenticalChannels)
{
    constexpr Color same{.red = 1, .green = 2, .blue = 3, .alpha = 4};

    EXPECT_EQ(kReference, same);
}

TEST(ColorTest, OperatorEquals_IsFalseWhenTheRedChannelDiffers)
{
    constexpr Color other{.red = 9, .green = 2, .blue = 3, .alpha = 4};

    EXPECT_NE(kReference, other);
}

TEST(ColorTest, OperatorEquals_IsFalseWhenTheGreenChannelDiffers)
{
    constexpr Color other{.red = 1, .green = 9, .blue = 3, .alpha = 4};

    EXPECT_NE(kReference, other);
}

TEST(ColorTest, OperatorEquals_IsFalseWhenTheBlueChannelDiffers)
{
    constexpr Color other{.red = 1, .green = 2, .blue = 9, .alpha = 4};

    EXPECT_NE(kReference, other);
}

TEST(ColorTest, OperatorEquals_IsFalseWhenTheAlphaChannelDiffers)
{
    constexpr Color other{.red = 1, .green = 2, .blue = 3, .alpha = 9};

    EXPECT_NE(kReference, other);
}
