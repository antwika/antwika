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

// One button, so asking for the opposite lets the run go again.
TEST(PauseStateTest, Set_LetsAHeldSimulationRunAgain)
{
    PauseState pause;
    pause.set(true);

    pause.set(false);

    EXPECT_FALSE(pause.paused());
}

// The whole reason the value is absolute rather than a toggle.
// Two players asking for a pause on one tick agree on one.
// A toggle would leave the run going, and both of them surprised.
TEST(PauseStateTest, Set_IsIdempotentSoTwoAsksDoNotCancelOut)
{
    PauseState pause;

    pause.set(true);
    pause.set(true);

    EXPECT_TRUE(pause.paused());
}

// And the same read backwards, for two players resuming at once.
TEST(PauseStateTest, Set_LeavesARunningSimulationRunning)
{
    PauseState pause;

    pause.set(false);
    pause.set(false);

    EXPECT_FALSE(pause.paused());
}
