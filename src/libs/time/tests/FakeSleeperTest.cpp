#include <gtest/gtest.h>

#include <chrono>

#include <antwika/time/fakes/FakeSleeper.hpp>

using antwika::time::fakes::FakeSleeper;
using namespace std::chrono_literals;

TEST(FakeSleeperTest, Requested_IsEmptyBeforeAnySleep)
{
    const FakeSleeper sleeper;

    EXPECT_TRUE(sleeper.requested().empty());
    EXPECT_EQ(sleeper.total(), 0ms);
}

TEST(FakeSleeperTest, Sleep_RecordsEveryDurationInOrder)
{
    FakeSleeper sleeper;

    sleeper.sleep(80ms);
    sleeper.sleep(20ms);

    EXPECT_EQ(
        sleeper.requested(),
        (std::vector<std::chrono::milliseconds>{80ms, 20ms}));
}

TEST(FakeSleeperTest, Total_AddsUpEveryDuration)
{
    FakeSleeper sleeper;

    sleeper.sleep(80ms);
    sleeper.sleep(20ms);

    EXPECT_EQ(sleeper.total(), 100ms);
}
