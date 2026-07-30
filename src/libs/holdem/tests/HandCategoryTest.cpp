#include <gtest/gtest.h>

#include <antwika/holdem/HandCategory.hpp>

using antwika::holdem::HandCategory;
using antwika::holdem::toString;

TEST(HandCategoryTest, ToString_NamesEveryCategory)
{
    EXPECT_EQ(toString(HandCategory::HighCard), "High Card");
    EXPECT_EQ(toString(HandCategory::OnePair), "One Pair");
    EXPECT_EQ(toString(HandCategory::TwoPair), "Two Pair");
    EXPECT_EQ(toString(HandCategory::ThreeOfAKind), "Three of a Kind");
    EXPECT_EQ(toString(HandCategory::Straight), "Straight");
    EXPECT_EQ(toString(HandCategory::Flush), "Flush");
    EXPECT_EQ(toString(HandCategory::FullHouse), "Full House");
    EXPECT_EQ(toString(HandCategory::FourOfAKind), "Four of a Kind");
    EXPECT_EQ(toString(HandCategory::StraightFlush), "Straight Flush");
}

TEST(HandCategoryTest, ToString_FallsBackForAValueThatNamesNoCategory)
{
    EXPECT_EQ(toString(static_cast<HandCategory>(42)), "Unknown");
}
