#include <gtest/gtest.h>

#include <antwika/holdem/ActionType.hpp>

using antwika::holdem::ActionType;
using antwika::holdem::toString;

TEST(ActionTypeTest, ToString_NamesEveryActionType)
{
    EXPECT_EQ(toString(ActionType::Fold), "fold");
    EXPECT_EQ(toString(ActionType::Check), "check");
    EXPECT_EQ(toString(ActionType::Call), "call");
    EXPECT_EQ(toString(ActionType::Bet), "bet");
    EXPECT_EQ(toString(ActionType::Raise), "raise");
}

TEST(ActionTypeTest, ToString_FallsBackForAValueThatNamesNoAction)
{
    EXPECT_EQ(toString(static_cast<ActionType>(42)), "unknown");
}
