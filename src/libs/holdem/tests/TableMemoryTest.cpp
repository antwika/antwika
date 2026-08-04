#include <gtest/gtest.h>

#include <antwika/rng/SplitMix64Rng.hpp>

#include "antwika/holdem/Action.hpp"
#include "antwika/holdem/Blinds.hpp"
#include "antwika/holdem/Deck.hpp"
#include "antwika/holdem/Table.hpp"
#include "antwika/holdem/TableMemory.hpp"
#include "antwika/holdem/TableStateError.hpp"

using antwika::holdem::Blinds;
using antwika::holdem::call;
using antwika::holdem::check;
using antwika::holdem::Deck;
using antwika::holdem::DeckMemory;
using antwika::holdem::makeSeatId;
using antwika::holdem::Table;
using antwika::holdem::TableMemory;
using antwika::holdem::TableStateError;
using antwika::rng::SplitMix64Rng;

namespace
{
    constexpr Blinds kBlinds{.small = 5, .big = 10};

    void sitTwo(Table &table)
    {
        table.seatPlayer(makeSeatId(0), 1000);
        table.seatPlayer(makeSeatId(1), 1000);
    }
} // namespace

// The whole point: a remembered table, deck and generator stood back
// up continue the session move for move, mid-hand included.
TEST(TableMemoryTest, Restore_ContinuesExactlyWhereItStood)
{
    SplitMix64Rng originalBits(9);
    Deck originalDeck(originalBits);
    Table original(2, kBlinds);
    sitTwo(original);
    original.startHand(originalDeck);
    original.apply(call());

    const auto tableMemory = original.remember();
    const auto deckMemory = originalDeck.remember();
    const auto bits = originalBits.currentState();

    SplitMix64Rng resumedBits(bits);
    Deck resumedDeck(resumedBits);
    resumedDeck.restore(deckMemory);
    Table resumed(2, kBlinds);
    resumed.restore(tableMemory, resumedDeck);

    EXPECT_EQ(resumed.remember(), tableMemory);

    // The same decisions walk both hands to the same end.
    original.apply(check());
    resumed.apply(check());

    while (original.isHandInProgress())
    {
        original.apply(check());
        resumed.apply(check());
    }

    EXPECT_FALSE(resumed.isHandInProgress());
    EXPECT_EQ(resumed.remember(), original.remember());

    // And the next hand shuffles off the very same stream.
    original.startHand(originalDeck);
    resumed.startHand(resumedDeck);

    EXPECT_EQ(resumed.remember(), original.remember());
    EXPECT_EQ(resumedDeck.remember(), originalDeck.remember());
}

TEST(TableMemoryTest, Restore_BetweenHandsHoldsNoDeck)
{
    SplitMix64Rng bits(11);
    Deck deck(bits);
    Table played(2, kBlinds);
    sitTwo(played);
    played.startHand(deck);
    played.apply(call());
    played.apply(check());

    while (played.isHandInProgress())
    {
        played.apply(check());
    }

    const auto memory = played.remember();

    SplitMix64Rng resumedBits(bits.currentState());
    Deck resumedDeck(resumedBits);
    resumedDeck.restore(deck.remember());
    Table resumed(2, kBlinds);
    resumed.restore(memory, resumedDeck);

    EXPECT_FALSE(resumed.isHandInProgress());
    EXPECT_EQ(resumed.remember(), memory);
    EXPECT_EQ(resumed.lastResult().pot, played.lastResult().pot);
}

TEST(TableMemoryTest, Restore_RefusesAnotherTablesSeatCount)
{
    SplitMix64Rng bits(3);
    Deck deck(bits);
    Table small(2, kBlinds);
    sitTwo(small);

    Table wide(4, kBlinds);

    EXPECT_THROW(
        wide.restore(small.remember(), deck), TableStateError);
}

TEST(TableMemoryTest, Restore_RefusesAHandInProgressWithNobodyToAct)
{
    SplitMix64Rng bits(3);
    Deck deck(bits);
    Table table(2, kBlinds);
    sitTwo(table);

    auto memory = table.remember();
    memory.handInProgress = true;
    memory.toAct.reset();

    EXPECT_THROW(table.restore(memory, deck), TableStateError);
}

TEST(TableMemoryTest, DeckMemory_ResumesTheDealExactly)
{
    SplitMix64Rng bits(21);
    Deck original(bits);
    original.shuffle();
    (void)original.deal();
    (void)original.deal();

    SplitMix64Rng otherBits(0);
    Deck resumed(otherBits);
    resumed.restore(original.remember());

    EXPECT_EQ(resumed.remaining(), original.remaining());
    EXPECT_EQ(resumed.deal(), original.deal());
    EXPECT_EQ(resumed.deal(), original.deal());
}

TEST(TableMemoryTest, EqualityComparesEveryField)
{
    SplitMix64Rng bits(5);
    Deck deck(bits);
    Table table(2, kBlinds);
    sitTwo(table);
    table.startHand(deck);

    const auto base = table.remember();

    auto moved = base;
    moved.pot = 999;
    EXPECT_NE(base, moved);

    auto turned = base;
    turned.handCount = 42;
    EXPECT_NE(base, turned);
}
