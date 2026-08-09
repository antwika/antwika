#include <gtest/gtest.h>

#include "antwika/input/Offset.hpp"

using antwika::input::Offset;

TEST(OffsetTest, Ctor_DefaultsToZeroOnBothAxes)
{
    constexpr Offset offset;

    EXPECT_EQ(offset.x, 0);
    EXPECT_EQ(offset.y, 0);
}

TEST(OffsetTest, OperatorEquals_ComparesBothAxesIndependently)
{
    constexpr Offset offset{.x = 3, .y = -4};

    EXPECT_EQ(offset, (Offset{.x = 3, .y = -4}));
    EXPECT_NE(offset, (Offset{.x = 3, .y = 4}));
    EXPECT_NE(offset, (Offset{.x = -3, .y = -4}));
}

TEST(OffsetTest, Ctor_HoldsNegativeAmountsOnBothAxes)
{
    constexpr Offset offset{.x = -2000, .y = -1};

    EXPECT_EQ(offset.x, -2000);
    EXPECT_EQ(offset.y, -1);
}
