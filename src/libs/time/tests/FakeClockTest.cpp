#include <gtest/gtest.h>

#include "antwika/time/fakes/FakeClock.hpp"

using antwika::time::fakes::FakeClock;

TEST(FakeClockTest, Now_ReportsWhatItWasConstructedWith)
{
    auto nowTime = std::chrono::system_clock::now();
    FakeClock clock{nowTime};
    EXPECT_EQ(clock.currentTime(), nowTime);
}

TEST(FakeClockTest, Advance_MovesTheReportedTimeForward)
{
    auto nowTime = std::chrono::system_clock::now();
    FakeClock clock{nowTime};
    std::chrono::seconds s{5};
    clock.advance(s);
    EXPECT_EQ(clock.currentTime(), nowTime + s);
}

TEST(FakeClockTest, Set_ReplacesTheReportedTime)
{
    auto nowTime = std::chrono::system_clock::now();
    FakeClock clock{nowTime};
    auto updatedNow = nowTime + std::chrono::seconds{5};
    clock.set(updatedNow);
    EXPECT_EQ(clock.currentTime(), updatedNow);
}
