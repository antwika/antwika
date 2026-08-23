#include <gtest/gtest.h>

#include <chrono>

#include <antwika/time/fakes/FakeClock.hpp>

#include "antwika/app/FramePacing.hpp"
#include "antwika/app/TickDebt.hpp"

using antwika::app::kTickPeriod;
using antwika::app::TickDebt;
using antwika::time::fakes::FakeClock;

TEST(TickDebtTest, Advance_ReadsTheTimeSinceTheFrameBefore)
{
    FakeClock clock{{}};
    TickDebt debt(clock);

    debt.start();
    clock.advance(kTickPeriod);

    EXPECT_EQ(debt.advance(), kTickPeriod);
}

TEST(TickDebtTest, OwesTick_HoldsOnceAWholeTickHasPassed)
{
    FakeClock clock{{}};
    TickDebt debt(clock);

    debt.start();

    EXPECT_FALSE(debt.owesTick());

    clock.advance(kTickPeriod);
    (void)debt.advance();

    EXPECT_TRUE(debt.owesTick());
}

TEST(TickDebtTest, PayTick_LeavesTheRemainderBehind)
{
    FakeClock clock{{}};
    TickDebt debt(clock);

    debt.start();
    clock.advance(kTickPeriod * 2);
    (void)debt.advance();

    debt.payTick();

    EXPECT_TRUE(debt.owesTick());

    debt.payTick();

    EXPECT_FALSE(debt.owesTick());
}

TEST(TickDebtTest, Forgive_DropsWhatIsStillOwed)
{
    FakeClock clock{{}};
    TickDebt debt(clock);

    debt.start();
    clock.advance(kTickPeriod * 10);
    (void)debt.advance();

    debt.forgive();

    EXPECT_FALSE(debt.owesTick());
    EXPECT_EQ(debt.getOwedTime(), std::chrono::nanoseconds{});
}

TEST(TickDebtTest, Start_ForgetsWhatAnEarlierRunOwed)
{
    FakeClock clock{{}};
    TickDebt debt(clock);

    debt.start();
    clock.advance(kTickPeriod * 3);
    (void)debt.advance();

    debt.start();

    EXPECT_FALSE(debt.owesTick());
}

TEST(TickDebtTest, Advance_AddsUpAcrossFrames)
{
    FakeClock clock{{}};
    TickDebt debt(clock);

    debt.start();

    for (int frame = 0; frame < 3; ++frame)
    {
        clock.advance(kTickPeriod / 2);
        (void)debt.advance();
    }

    EXPECT_EQ(debt.getOwedTime(), (kTickPeriod * 3) / 2);
}
