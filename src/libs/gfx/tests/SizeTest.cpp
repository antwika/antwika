#include <gtest/gtest.h>

#include "antwika/gfx/Size.hpp"

using antwika::gfx::Size;

namespace
{
    constexpr Size kReferenceSize{.width = 640, .height = 480};
}

TEST(SizeTest, OperatorEquals_IsTrueForIdenticalDimensions)
{
    constexpr Size sameSize{.width = 640, .height = 480};

    EXPECT_EQ(kReferenceSize, sameSize);
}

TEST(SizeTest, OperatorEquals_IsFalseWhenTheWidthDiffers)
{
    constexpr Size otherSize{.width = 999, .height = 480};

    EXPECT_NE(kReferenceSize, otherSize);
}

TEST(SizeTest, OperatorEquals_IsFalseWhenTheHeightDiffers)
{
    constexpr Size otherSize{.width = 640, .height = 999};

    EXPECT_NE(kReferenceSize, otherSize);
}
