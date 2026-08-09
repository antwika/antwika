#include <gtest/gtest.h>

#include "antwika/game/GameState.hpp"

using antwika::game::GameState;
using antwika::game::kStartingMoney;

TEST(GameStateTest, Ctor_DefaultsEveryField)
{
    GameState state;
    EXPECT_EQ(state.ticksProcessed, 0);
    EXPECT_EQ(state.score, 0);

    EXPECT_EQ(state.money, kStartingMoney);
}

TEST(GameStateTest, OperatorEquals_MatchesAnIdenticalState)
{
    GameState state1{.ticksProcessed = 2, .score = 5};
    GameState state2{.ticksProcessed = 2, .score = 5};
    EXPECT_EQ(state1, state2);
}

TEST(GameStateTest, OperatorEquals_SeparatesDifferentTicks)
{
    GameState state1{.ticksProcessed = 1, .score = 5};
    GameState state2{.ticksProcessed = 2, .score = 5};
    EXPECT_NE(state1, state2);
}

TEST(GameStateTest, OperatorEquals_SeparatesADifferentScore)
{
    GameState state1{.ticksProcessed = 2, .score = 5};
    GameState state2{.ticksProcessed = 2, .score = 6};
    EXPECT_NE(state1, state2);
}

TEST(GameStateTest, OperatorEquals_SeparatesDifferentMoney)
{
    GameState state1{.ticksProcessed = 2, .score = 5, .money = 100};
    GameState state2{.ticksProcessed = 2, .score = 5, .money = 200};
    EXPECT_NE(state1, state2);
}
