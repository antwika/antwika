#include <gtest/gtest.h>

#include <chrono>

#include "antwika/time/ThreadSleeper.hpp"

using antwika::time::ThreadSleeper;
using namespace std::chrono_literals;

TEST(ThreadSleeperTest, Sleep_ReturnsForAZeroDuration)
{
    ThreadSleeper sleeper;

    EXPECT_NO_THROW(sleeper.sleep(0ms));
}

TEST(ThreadSleeperTest, Sleep_ReturnsForANegativeDuration)
{
    ThreadSleeper sleeper;

    EXPECT_NO_THROW(sleeper.sleep(-5ms));
}

TEST(ThreadSleeperTest, Sleep_WaitsAtLeastAsLongAsItWasAsked)
{
    ThreadSleeper sleeper;

    const auto beforeTime = std::chrono::steady_clock::now();
    sleeper.sleep(20ms);
    const auto elapsedTime = std::chrono::steady_clock::now() - beforeTime;

    EXPECT_GE(elapsedTime, 20ms);
}
