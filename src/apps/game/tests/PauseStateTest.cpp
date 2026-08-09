#include <gtest/gtest.h>

#include "antwika/game/PauseState.hpp"

using antwika::game::PauseState;

TEST(PauseStateTest, Paused_IsFalseUntilSomebodyAsksForOne)
{
    const PauseState pause;

    EXPECT_FALSE(pause.paused());
}

TEST(PauseStateTest, Set_HoldsTheSimulationStill)
{
    PauseState pause;

    pause.set(true);

    EXPECT_TRUE(pause.paused());
}

TEST(PauseStateTest, Set_LetsAHeldSimulationRunAgain)
{
    PauseState pause;
    pause.set(true);

    pause.set(false);

    EXPECT_FALSE(pause.paused());
}

TEST(PauseStateTest, Set_IsIdempotentSoTwoAsksDoNotCancelOut)
{
    PauseState pause;

    pause.set(true);
    pause.set(true);

    EXPECT_TRUE(pause.paused());
}

TEST(PauseStateTest, Set_LeavesARunningSimulationRunning)
{
    PauseState pause;

    pause.set(false);
    pause.set(false);

    EXPECT_FALSE(pause.paused());
}
