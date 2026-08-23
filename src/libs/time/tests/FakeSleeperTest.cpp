#include <gtest/gtest.h>

#include <chrono>

#include <antwika/time/fakes/FakeClock.hpp>
#include <antwika/time/fakes/FakeSleeper.hpp>

using antwika::time::fakes::FakeClock;
using antwika::time::fakes::FakeSleeper;
using namespace std::chrono_literals;

TEST(FakeSleeperTest, Requested_IsEmptyBeforeAnySleep)
{
    const FakeSleeper sleeper;

    EXPECT_TRUE(sleeper.getRequestedSpans().empty());
    EXPECT_EQ(sleeper.getTotal(), 0ms);
}

TEST(FakeSleeperTest, Sleep_RecordsEveryDurationInOrder)
{
    FakeSleeper sleeper;

    sleeper.sleep(80ms);
    sleeper.sleep(20ms);

    EXPECT_EQ(
        sleeper.getRequestedSpans(),
        (std::vector<std::chrono::milliseconds>{80ms, 20ms}));
}

TEST(FakeSleeperTest, Total_AddsUpEveryDuration)
{
    FakeSleeper sleeper;

    sleeper.sleep(80ms);
    sleeper.sleep(20ms);

    EXPECT_EQ(sleeper.getTotal(), 100ms);
}

TEST(FakeSleeperTest, Sleep_CarriesAClockItWasGivenForward)
{
    const auto start = std::chrono::system_clock::time_point{};
    FakeClock clock(start);
    FakeSleeper sleeper(clock);

    sleeper.sleep(80ms);
    sleeper.sleep(20ms);

    EXPECT_EQ(clock.getCurrentTime(), start + 100ms);
}
