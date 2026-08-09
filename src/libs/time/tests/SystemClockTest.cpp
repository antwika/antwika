#include <gtest/gtest.h>

#include <chrono>

#include "antwika/time/SystemClock.hpp"

using antwika::time::SystemClock;

TEST(SystemClockTest, Now_ReportsTheSystemClock)
{
    const SystemClock clock;

    const auto first = clock.now();
    const auto second = clock.now();

    EXPECT_GT(first.time_since_epoch().count(), 0);
    EXPECT_GE(second, first);
}

TEST(SystemClockTest, Now_ReportsATimeThisBuildCouldNotPredate)
{
    const auto releasedAt = std::chrono::system_clock::time_point{}
        + std::chrono::seconds{1577836800};

    EXPECT_GT(SystemClock().now(), releasedAt);
}
