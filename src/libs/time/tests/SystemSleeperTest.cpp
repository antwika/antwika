#include <gtest/gtest.h>

#include <chrono>

#include "antwika/time/SystemSleeper.hpp"

using antwika::time::SystemSleeper;
using namespace std::chrono_literals;

TEST(SystemSleeperTest, Sleep_ReturnsForAZeroDuration)
{
    SystemSleeper sleeper;

    EXPECT_NO_THROW(sleeper.sleep(0ms));
}

TEST(SystemSleeperTest, Sleep_ReturnsForANegativeDuration)
{
    SystemSleeper sleeper;

    EXPECT_NO_THROW(sleeper.sleep(-5ms));
}

TEST(SystemSleeperTest, Sleep_WaitsAtLeastAsLongAsItWasAsked)
{
    SystemSleeper sleeper;

    const auto before = std::chrono::steady_clock::now();
    sleeper.sleep(20ms);
    const auto elapsed = std::chrono::steady_clock::now() - before;

    EXPECT_GE(elapsed, 20ms);
}
