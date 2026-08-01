#include <gtest/gtest.h>

#include "antwika/game/PauseState.hpp"

using antwika::game::PauseState;

TEST(PauseStateTest, Paused_IsFalseUntilSomethingTogglesIt)
{
    const PauseState pause;

    EXPECT_FALSE(pause.paused());
}

TEST(PauseStateTest, Toggle_HoldsTheSimulationStill)
{
    PauseState pause;

    pause.toggle();

    EXPECT_TRUE(pause.paused());
}

// One button, so the second press is what lets the run go again.
TEST(PauseStateTest, Toggle_LetsItGoAgainOnTheSecondCall)
{
    PauseState pause;

    pause.toggle();
    pause.toggle();

    EXPECT_FALSE(pause.paused());
}

TEST(PauseStateTest, Release_LetsAHeldSimulationRunAgain)
{
    PauseState pause;
    pause.hold();

    pause.release();

    EXPECT_FALSE(pause.paused());
}

// hold()'s counterpart, and unlike toggle() it says which way it goes.
TEST(PauseStateTest, Release_LeavesARunningSimulationRunning)
{
    PauseState pause;

    pause.release();

    EXPECT_FALSE(pause.paused());
}
