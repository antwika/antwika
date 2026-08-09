#include <gtest/gtest.h>

#include "antwika/gfx/Size.hpp"

using antwika::gfx::Size;

namespace
{
    constexpr Size kReference{.width = 640, .height = 480};
}

TEST(SizeTest, OperatorEquals_IsTrueForIdenticalDimensions)
{
    constexpr Size same{.width = 640, .height = 480};

    EXPECT_EQ(kReference, same);
}

TEST(SizeTest, OperatorEquals_IsFalseWhenTheWidthDiffers)
{
    constexpr Size other{.width = 999, .height = 480};

    EXPECT_NE(kReference, other);
}

TEST(SizeTest, OperatorEquals_IsFalseWhenTheHeightDiffers)
{
    constexpr Size other{.width = 640, .height = 999};

    EXPECT_NE(kReference, other);
}
