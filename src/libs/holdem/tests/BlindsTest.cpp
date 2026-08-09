#include <gtest/gtest.h>

#include <antwika/holdem/Blinds.hpp>

using antwika::holdem::Blinds;

TEST(BlindsTest, OperatorEquals_ComparesEveryFieldIndependently)
{
    constexpr Blinds levels{.small = 5, .big = 10};

    EXPECT_EQ(levels, (Blinds{.small = 5, .big = 10}));
    EXPECT_NE(levels, (Blinds{.small = 1, .big = 10}));
    EXPECT_NE(levels, (Blinds{.small = 5, .big = 20}));
}
