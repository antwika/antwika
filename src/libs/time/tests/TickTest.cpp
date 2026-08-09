#include <gtest/gtest.h>

#include <cstdint>
#include <limits>

#include "antwika/time/Tick.hpp"

using antwika::time::Tick;

TEST(TickTest, Increment_CountsUpFromZero)
{
    Tick tick{0};
    ++tick;
    EXPECT_EQ(tick, 1);
}

TEST(TickTest, Max_SupportsTheFullUint64Range)
{
    constexpr Tick maxTick = std::numeric_limits<Tick>::max();
    EXPECT_EQ(maxTick, std::numeric_limits<std::uint64_t>::max());
}
