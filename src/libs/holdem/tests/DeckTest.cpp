#include <gtest/gtest.h>

#include <cstddef>
#include <set>
#include <vector>

#include <antwika/holdem/Card.hpp>
#include <antwika/holdem/Deck.hpp>
#include <antwika/holdem/DeckExhaustedError.hpp>
#include <antwika/rng/SplitMix64Rng.hpp>

#include <antwika/rng/fakes/FakeRng.hpp>

using antwika::holdem::Card;
using antwika::holdem::Deck;
using antwika::holdem::DeckExhaustedError;
using antwika::holdem::kCardCount;
using antwika::rng::SplitMix64Rng;
using antwika::rng::fakes::FakeRng;

namespace
{
    [[nodiscard]] std::vector<Card> dealAll(Deck &deck)
    {
        std::vector<Card> cards;
        for (std::size_t index = 0; index < kCardCount; ++index)
        {
            cards.push_back(deck.deal());
        }
        return cards;
    }
} // namespace

TEST(DeckTest, Deal_HandsOutEveryCardExactlyOnce)
{
    SplitMix64Rng rng(7);
    Deck deck(rng);
    deck.shuffle();

    const auto cards = dealAll(deck);
    const std::set<Card> distinct(cards.begin(), cards.end());
    EXPECT_EQ(distinct.size(), kCardCount);
}

TEST(DeckTest, Remaining_CountsDownAsCardsAreDealt)
{
    SplitMix64Rng rng(7);
    Deck deck(rng);
    deck.shuffle();

    EXPECT_EQ(deck.remaining(), kCardCount);
    const auto ignored = deck.deal();
    static_cast<void>(ignored);
    EXPECT_EQ(deck.remaining(), kCardCount - 1);
}

TEST(DeckTest, Deal_RefusesToDealPastTheEndOfTheDeck)
{
    SplitMix64Rng rng(7);
    Deck deck(rng);
    deck.shuffle();
    static_cast<void>(dealAll(deck));

    EXPECT_EQ(deck.remaining(), 0U);
    EXPECT_THROW(static_cast<void>(deck.deal()), DeckExhaustedError);
}

TEST(DeckTest, Shuffle_ReturnsEveryDealtCardToTheDeck)
{
    SplitMix64Rng rng(7);
    Deck deck(rng);
    deck.shuffle();
    static_cast<void>(dealAll(deck));

    deck.shuffle();
    EXPECT_EQ(deck.remaining(), kCardCount);
}

TEST(DeckTest, Shuffle_DealsTheSameOrderForTheSameSeed)
{
    SplitMix64Rng first(2026);
    Deck firstDeck(first);
    firstDeck.shuffle();

    SplitMix64Rng second(2026);
    Deck secondDeck(second);
    secondDeck.shuffle();

    EXPECT_EQ(dealAll(firstDeck), dealAll(secondDeck));
}

TEST(DeckTest, Shuffle_DealsADifferentOrderOnEachSuccessiveShuffle)
{
    SplitMix64Rng rng(2026);
    Deck deck(rng);
    deck.shuffle();
    const auto first = dealAll(deck);
    deck.shuffle();
    const auto second = dealAll(deck);

    EXPECT_NE(first, second);
}

// A generator stuck at zero always picks position 0.
// That is the one input where Fisher-Yates could lose a card.
TEST(DeckTest, Shuffle_KeepsAFullDeckEvenWhenEverySwapPicksTheSameSlot)
{
    FakeRng rng({0});
    Deck deck(rng);
    deck.shuffle();

    const auto cards = dealAll(deck);
    const std::set<Card> distinct(cards.begin(), cards.end());
    EXPECT_EQ(distinct.size(), kCardCount);
}
