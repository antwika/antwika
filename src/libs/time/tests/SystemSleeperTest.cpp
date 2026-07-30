#include <gtest/gtest.h>

#include <chrono>

#include "antwika/time/SystemSleeper.hpp"

using antwika::time::SystemSleeper;
using namespace std::chrono_literals;

// Nothing here asserts elapsed time.
// A test that did would flake on a loaded machine and prove nothing.
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
