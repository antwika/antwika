#include <gtest/gtest.h>

#include "antwika/gfx/Size.hpp"

using antwika::gfx::Size;

namespace
{
    constexpr Size kReference{.width = 640, .height = 480};
} // namespace

TEST(SizeTest, Equality_IsTrueForIdenticalDimensions)
{
    constexpr Size same{.width = 640, .height = 480};

    EXPECT_EQ(kReference, same);
}

TEST(SizeTest, Equality_IsFalseWhenTheWidthDiffers)
{
    constexpr Size other{.width = 999, .height = 480};

    EXPECT_NE(kReference, other);
}

TEST(SizeTest, Equality_IsFalseWhenTheHeightDiffers)
{
    constexpr Size other{.width = 640, .height = 999};

    EXPECT_NE(kReference, other);
}
