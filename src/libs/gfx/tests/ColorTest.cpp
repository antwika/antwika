#include <gtest/gtest.h>

#include "antwika/gfx/Color.hpp"

using antwika::gfx::Color;

namespace
{
    constexpr Color kReferenceColor{
        .red = 1,
        .green = 2,
        .blue = 3,
        .alpha = 4};
}

TEST(ColorTest, OperatorEquals_IsTrueForIdenticalChannels)
{
    constexpr Color sameColor{.red = 1, .green = 2, .blue = 3, .alpha = 4};

    EXPECT_EQ(kReferenceColor, sameColor);
}

TEST(ColorTest, OperatorEquals_IsFalseWhenTheRedChannelDiffers)
{
    constexpr Color otherColor{.red = 9, .green = 2, .blue = 3, .alpha = 4};

    EXPECT_NE(kReferenceColor, otherColor);
}

TEST(ColorTest, OperatorEquals_IsFalseWhenTheGreenChannelDiffers)
{
    constexpr Color otherColor{.red = 1, .green = 9, .blue = 3, .alpha = 4};

    EXPECT_NE(kReferenceColor, otherColor);
}

TEST(ColorTest, OperatorEquals_IsFalseWhenTheBlueChannelDiffers)
{
    constexpr Color otherColor{.red = 1, .green = 2, .blue = 9, .alpha = 4};

    EXPECT_NE(kReferenceColor, otherColor);
}

TEST(ColorTest, OperatorEquals_IsFalseWhenTheAlphaChannelDiffers)
{
    constexpr Color otherColor{.red = 1, .green = 2, .blue = 3, .alpha = 9};

    EXPECT_NE(kReferenceColor, otherColor);
}
