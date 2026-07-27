#include <gtest/gtest.h>

#include <limits>

#include "antwika/time/Tick.hpp"

using antwika::time::Tick;

TEST(TickTest, IsAZeroBasedUnsignedCounter)
{
    Tick tick{0};
    ++tick;
    EXPECT_EQ(tick, 1);
}

TEST(TickTest, SupportsFullRangeOfUint64)
{
    constexpr Tick maxTick = std::numeric_limits<Tick>::max();
    EXPECT_GT(maxTick, 0);
}
