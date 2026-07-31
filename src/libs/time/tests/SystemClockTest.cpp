#include <gtest/gtest.h>

#include "antwika/time/SystemClock.hpp"

using antwika::time::SystemClock;

TEST(SystemClockTest, Now_ReportsTheSystemClock)
{
    SystemClock clock;
    EXPECT_NO_THROW((void)clock.now());
}
