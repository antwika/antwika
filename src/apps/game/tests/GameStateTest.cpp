#include <gtest/gtest.h>

#include "antwika/game/GameState.hpp"

using antwika::game::GameState;
using antwika::game::kStartingMoney;

TEST(GameStateTest, DefaultConstruction)
{
    GameState state;
    EXPECT_EQ(state.ticksProcessed, 0);
    EXPECT_EQ(state.score, 0);

    // A fresh session opens with the starting bank, by construction.
    EXPECT_EQ(state.money, kStartingMoney);
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

TEST(GameStateTest, InequalityWhenOnlyMoneyDiffers)
{
    GameState state1{.ticksProcessed = 2, .score = 5, .money = 100};
    GameState state2{.ticksProcessed = 2, .score = 5, .money = 200};
    EXPECT_NE(state1, state2);
}
