#include <gtest/gtest.h>

#include "antwika/game/GameState.hpp"

using antwika::game::GameState;

TEST(GameStateTest, DefaultConstruction)
{
    GameState state;
    EXPECT_EQ(state.ticksProcessed, 0);
    EXPECT_EQ(state.score, 0);
}

TEST(GameStateTest, Equality)
{
    GameState state1{.ticksProcessed = 2, .score = 5};
    GameState state2{.ticksProcessed = 2, .score = 5};
    EXPECT_EQ(state1, state2);
}

TEST(GameStateTest, InequalityWhenOnlyTicksProcessedDiffers)
{
    GameState state1{.ticksProcessed = 1, .score = 5};
    GameState state2{.ticksProcessed = 2, .score = 5};
    EXPECT_NE(state1, state2);
}

TEST(GameStateTest, InequalityWhenOnlyScoreDiffers)
{
    GameState state1{.ticksProcessed = 2, .score = 5};
    GameState state2{.ticksProcessed = 2, .score = 6};
    EXPECT_NE(state1, state2);
}
