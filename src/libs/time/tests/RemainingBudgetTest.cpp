#include <gtest/gtest.h>

#include <chrono>

#include <antwika/time/RemainingBudget.hpp>
#include <antwika/time/fakes/FakeClock.hpp>
#include <antwika/time/fakes/FakeSleeper.hpp>

using antwika::time::remainingOf;
using antwika::time::fakes::FakeClock;
using antwika::time::fakes::FakeSleeper;
using namespace std::chrono_literals;

namespace
{

    constexpr std::chrono::milliseconds kBudget{16};

}

TEST(RemainingBudgetTest, RemainingOf_LeavesTheWholeBudgetForNoWorkAtAll)
{
    EXPECT_EQ(remainingOf(kBudget, 0ns), kBudget);
}

TEST(RemainingBudgetTest, RemainingOf_TakesTheWorkOffTheBudget)
{
    EXPECT_EQ(remainingOf(kBudget, 6ms), 10ms);
}

TEST(RemainingBudgetTest, RemainingOf_LeavesNothingOfABudgetRunOver)
{
    EXPECT_EQ(remainingOf(kBudget, 20ms), 0ms);
}

TEST(RemainingBudgetTest, RemainingOf_LeavesNothingOfABudgetSpentToTheLast)
{
    EXPECT_EQ(remainingOf(kBudget, 16ms), 0ms);
}

TEST(RemainingBudgetTest, RemainingOf_RoundsAPartMillisecondUp)
{
    EXPECT_EQ(remainingOf(kBudget, 15'400'000ns), 1ms);
    EXPECT_EQ(remainingOf(kBudget, 100'000ns), 16ms);
}

TEST(RemainingBudgetTest, RemainingOf_LeavesTheWholeBudgetForAClockStepBack)
{
    EXPECT_EQ(remainingOf(kBudget, -5ms), kBudget);
}

TEST(RemainingBudgetTest, RemainingOf_HoldsEveryFrameToItsBudget)
{
    FakeClock watch{std::chrono::system_clock::time_point{}};
    FakeSleeper sleeper;

    for (const auto work : {3ms, 12ms, 30ms})
    {
        const auto startedAt = watch.now();

        watch.advance(work);
        sleeper.sleep(remainingOf(kBudget, watch.now() - startedAt));
    }

    EXPECT_EQ(
        sleeper.requested(),
        (std::vector<std::chrono::milliseconds>{13ms, 4ms, 0ms}));
    EXPECT_EQ(sleeper.total(), 17ms);
}
