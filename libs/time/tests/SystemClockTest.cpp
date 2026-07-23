#include <gtest/gtest.h>

#include "antwika/time/SystemClock.hpp"

using antwika::time::SystemClock;

TEST(SystemClockTest, Now) {
    SystemClock clock;
    auto now = clock.now();
}
