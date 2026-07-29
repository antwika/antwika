#include <gtest/gtest.h>

#include <vector>

#include <antwika/holdem/Card.hpp>
#include <antwika/holdem/CardFormatError.hpp>
#include <antwika/holdem/CardText.hpp>

using antwika::holdem::Card;
using antwika::holdem::CardFormatError;
using antwika::holdem::makeCard;
using antwika::holdem::parseCard;
using antwika::holdem::parseCards;
using antwika::holdem::Rank;
using antwika::holdem::Suit;
using antwika::holdem::toString;

TEST(CardTextTest, ToString_WritesRankThenSuit)
{
    EXPECT_EQ(toString(makeCard(Rank::Ace, Suit::Spades)), "As");
    EXPECT_EQ(toString(makeCard(Rank::Ten, Suit::Diamonds)), "Td");
    EXPECT_EQ(toString(makeCard(Rank::Two, Suit::Clubs)), "2c");
    EXPECT_EQ(toString(makeCard(Rank::King, Suit::Hearts)), "Kh");
}

TEST(CardTextTest, ParseCard_RoundTripsEveryCard)
{
    for (std::uint8_t value = 0; value < antwika::holdem::kCardCount;
         ++value)
    {
        const auto card = static_cast<Card>(value);
        EXPECT_EQ(parseCard(toString(card)), card);
    }
}

TEST(CardTextTest, ParseCard_AcceptsEitherCase)
{
    EXPECT_EQ(parseCard("as"), makeCard(Rank::Ace, Suit::Spades));
    EXPECT_EQ(parseCard("AS"), makeCard(Rank::Ace, Suit::Spades));
}

TEST(CardTextTest, ParseCard_RejectsTextThatIsNotTwoCharacters)
{
    EXPECT_THROW(static_cast<void>(parseCard("A")), CardFormatError);
    EXPECT_THROW(static_cast<void>(parseCard("Ase")), CardFormatError);
    EXPECT_THROW(static_cast<void>(parseCard("")), CardFormatError);
}

TEST(CardTextTest, ParseCard_RejectsAnUnknownRank)
{
    EXPECT_THROW(static_cast<void>(parseCard("1s")), CardFormatError);
}

TEST(CardTextTest, ParseCard_RejectsAnUnknownSuit)
{
    EXPECT_THROW(static_cast<void>(parseCard("Ax")), CardFormatError);
}

TEST(CardTextTest, ParseCards_ReadsAWhitespaceSeparatedList)
{
    const auto cards = parseCards("As Td 7c");
    ASSERT_EQ(cards.size(), 3U);
    EXPECT_EQ(cards[0], makeCard(Rank::Ace, Suit::Spades));
    EXPECT_EQ(cards[1], makeCard(Rank::Ten, Suit::Diamonds));
    EXPECT_EQ(cards[2], makeCard(Rank::Seven, Suit::Clubs));
}

TEST(CardTextTest, ParseCards_ReadsNothingFromEmptyText)
{
    EXPECT_TRUE(parseCards("").empty());
}

TEST(CardTextTest, ParseCards_PropagatesAnInvalidEntry)
{
    EXPECT_THROW(static_cast<void>(parseCards("As Zz")), CardFormatError);
}

TEST(CardTextTest, ToString_JoinsASpanWithSpaces)
{
    const auto cards = parseCards("As Td 7c");
    EXPECT_EQ(toString(cards), "As Td 7c");
}

TEST(CardTextTest, ToString_WritesNothingForNoCards)
{
    const std::vector<Card> none;
    EXPECT_EQ(toString(none), "");
}
