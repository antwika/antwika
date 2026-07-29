#include <gtest/gtest.h>

#include <antwika/holdem/Action.hpp>
#include <antwika/holdem/ActionType.hpp>

using antwika::holdem::Action;
using antwika::holdem::ActionType;
using antwika::holdem::bet;
using antwika::holdem::call;
using antwika::holdem::check;
using antwika::holdem::fold;
using antwika::holdem::raiseTo;

TEST(ActionTest, Builders_TagEachActionWithItsType)
{
    EXPECT_EQ(fold().type, ActionType::Fold);
    EXPECT_EQ(check().type, ActionType::Check);
    EXPECT_EQ(call().type, ActionType::Call);
    EXPECT_EQ(bet(50).type, ActionType::Bet);
    EXPECT_EQ(raiseTo(200).type, ActionType::Raise);
}

TEST(ActionTest, Builders_LeaveTheAmountToTheTableUnlessWagering)
{
    EXPECT_EQ(fold().amount, 0U);
    EXPECT_EQ(check().amount, 0U);
    EXPECT_EQ(call().amount, 0U);
    EXPECT_EQ(bet(50).amount, 50U);
    EXPECT_EQ(raiseTo(200).amount, 200U);
}

TEST(ActionTest, Equality_ComparesTypeAndAmountTogether)
{
    EXPECT_EQ(bet(50), bet(50));
    EXPECT_NE(bet(50), bet(60));
    EXPECT_NE(bet(50), raiseTo(50));
}
