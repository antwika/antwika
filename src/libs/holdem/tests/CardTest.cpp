#include <gtest/gtest.h>

#include <cstdint>

#include <antwika/holdem/Card.hpp>

using antwika::holdem::Card;
using antwika::holdem::kCardCount;
using antwika::holdem::kRankCount;
using antwika::holdem::kSuitCount;
using antwika::holdem::makeCard;
using antwika::holdem::Rank;
using antwika::holdem::rankBit;
using antwika::holdem::rankOf;
using antwika::holdem::rawValue;
using antwika::holdem::Suit;
using antwika::holdem::suitOf;

TEST(CardTest, MakeCard_PacksEveryRankAndSuitIntoADistinctValue)
{
    bool seen[kCardCount] = {};
    for (std::uint8_t rank = 0; rank < kRankCount; ++rank)
    {
        for (std::uint8_t suit = 0; suit < kSuitCount; ++suit)
        {
            const auto card = makeCard(
                static_cast<Rank>(rank), static_cast<Suit>(suit));
            ASSERT_LT(rawValue(card), kCardCount);
            EXPECT_FALSE(seen[rawValue(card)]);
            seen[rawValue(card)] = true;
        }
    }
}

TEST(CardTest, RankOf_RecoversTheRankItWasPackedWith)
{
    const auto card = makeCard(Rank::Queen, Suit::Hearts);
    EXPECT_EQ(rankOf(card), Rank::Queen);
}

TEST(CardTest, SuitOf_RecoversTheSuitItWasPackedWith)
{
    const auto card = makeCard(Rank::Queen, Suit::Hearts);
    EXPECT_EQ(suitOf(card), Suit::Hearts);
}

TEST(CardTest, RawValue_OrdersCardsByRankAboveSuit)
{
    const auto lowRankHighSuit = makeCard(Rank::Two, Suit::Spades);
    const auto highRankLowSuit = makeCard(Rank::Three, Suit::Clubs);
    EXPECT_LT(rawValue(lowRankHighSuit), rawValue(highRankLowSuit));
}

TEST(CardTest, RankBit_SetsOnlyThatRanksBit)
{
    EXPECT_EQ(rankBit(Rank::Two), 0b1U);
    EXPECT_EQ(rankBit(Rank::Three), 0b10U);
    EXPECT_EQ(rankBit(Rank::Ace), 1U << 12U);
}

TEST(CardTest, RawValue_UnwrapsRanksAndSuitsToTheirBitPositions)
{
    EXPECT_EQ(rawValue(Rank::Two), 0);
    EXPECT_EQ(rawValue(Rank::Ace), 12);
    EXPECT_EQ(rawValue(Suit::Clubs), 0);
    EXPECT_EQ(rawValue(Suit::Spades), 3);
}
