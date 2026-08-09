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
using antwika::holdem::rawValue;
using antwika::holdem::Table;
using antwika::holdem::TableMemory;
using antwika::holdem::TableStateError;
using antwika::rng::SplitMix64Rng;

namespace
{
    constexpr Blinds kBlinds{.small = 5, .big = 10};
    constexpr int kMaxChecksToEndAHand = 16;

    void sitTwo(Table &table)
    {
        table.seatPlayer(makeSeatId(0), 1000);
        table.seatPlayer(makeSeatId(1), 1000);
    }
}

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

    original.apply(check());
    resumed.apply(check());

    int checks = 0;
    while (original.isHandInProgress())
    {
        ASSERT_LT(checks++, kMaxChecksToEndAHand);
        original.apply(check());
        resumed.apply(check());
    }

    EXPECT_FALSE(resumed.isHandInProgress());
    EXPECT_EQ(resumed.remember(), original.remember());

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

    int checks = 0;
    while (played.isHandInProgress())
    {
        ASSERT_LT(checks++, kMaxChecksToEndAHand);
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

TEST(TableMemoryTest, OperatorEquals_ComparesEveryField)
{
    SplitMix64Rng bits(5);
    Deck deck(bits);
    Table table(2, kBlinds);
    sitTwo(table);
    table.startHand(deck);

    const auto base = table.remember();

    const auto twin = base;
    EXPECT_EQ(base, twin);

    auto reseated = base;
    reseated.seats[0].stack += 1;
    EXPECT_NE(base, reseated);

    auto decided = base;
    antwika::holdem::HandResult won;
    won.pot = 1;
    decided.result = won;
    EXPECT_NE(base, decided);

    auto waiting = base;
    waiting.toAct = makeSeatId(1 - rawValue(*base.toAct));
    EXPECT_NE(base, waiting);

    auto moved = base;
    moved.pot = 999;
    EXPECT_NE(base, moved);

    auto raised = base;
    raised.betting.currentBet += 1;
    EXPECT_NE(base, raised);

    auto staged = base;
    staged.stage = antwika::holdem::Stage::River;
    EXPECT_NE(base, staged);

    auto dealt = base;
    dealt.board.push_back(static_cast<antwika::holdem::Card>(0));
    EXPECT_NE(base, dealt);

    auto turned = base;
    turned.handCount = 42;
    EXPECT_NE(base, turned);

    auto rotated = base;
    rotated.button = makeSeatId(1 - rawValue(base.button));
    EXPECT_NE(base, rotated);

    auto settled = base;
    settled.handInProgress = !base.handInProgress;
    EXPECT_NE(base, settled);
}

TEST(TableMemoryTest, OperatorEquals_ComparesDeckAndBettingFields)
{
    SplitMix64Rng bits(5);
    Deck deck(bits);
    deck.shuffle();

    const auto base = deck.remember();

    const auto twin = base;
    EXPECT_EQ(base, twin);

    auto reordered = base;
    std::swap(reordered.cards[0], reordered.cards[1]);
    EXPECT_NE(base, reordered);

    auto drawn = base;
    drawn.dealt += 1;
    EXPECT_NE(base, drawn);

    const antwika::holdem::BettingMemory betting{
        .currentBet = 10, .lastRaiseSize = 10};

    const auto bettingTwin = betting;
    EXPECT_EQ(betting, bettingTwin);

    auto pushed = betting;
    pushed.currentBet = 20;
    EXPECT_NE(betting, pushed);

    auto reopened = betting;
    reopened.lastRaiseSize = 20;
    EXPECT_NE(betting, reopened);
}

TEST(TableMemoryTest, SeatEquality_ComparesEveryField)
{
    antwika::holdem::Seat base;
    base.occupied = true;

    const auto twin = base;
    EXPECT_EQ(base, twin);

    auto richer = base;
    richer.stack += 1;
    EXPECT_NE(base, richer);

    auto staked = base;
    staked.committed += 1;
    EXPECT_NE(base, staked);

    auto called = base;
    called.roundCommitted += 1;
    EXPECT_NE(base, called);

    auto empty = base;
    empty.occupied = false;
    EXPECT_NE(base, empty);

    auto dealt = base;
    dealt.inHand = true;
    EXPECT_NE(base, dealt);

    auto acted = base;
    acted.actedThisRound = true;
    EXPECT_NE(base, acted);

    auto capped = base;
    capped.mayRaise = false;
    EXPECT_NE(base, capped);

    auto redealt = base;
    redealt.holeCards[0] = static_cast<antwika::holdem::Card>(51);
    EXPECT_NE(base, redealt);
}

TEST(TableMemoryTest, HandResultEquality_ComparesEveryField)
{
    const antwika::holdem::HandResult base{
        .pot = 20,
        .payouts = {{.seat = makeSeatId(0), .amount = 20}},
        .showdown = {{.seat = makeSeatId(0)}},
        .board = {static_cast<antwika::holdem::Card>(7)},
        .stage = antwika::holdem::Stage::River};

    const auto twin = base;
    EXPECT_EQ(base, twin);

    auto bigger = base;
    bigger.pot += 1;
    EXPECT_NE(base, bigger);

    auto repaid = base;
    repaid.payouts[0].amount += 1;
    EXPECT_NE(base, repaid);

    auto reshown = base;
    reshown.showdown.clear();
    EXPECT_NE(base, reshown);

    auto redealt = base;
    redealt.board.clear();
    EXPECT_NE(base, redealt);

    auto earlier = base;
    earlier.stage = antwika::holdem::Stage::Turn;
    EXPECT_NE(base, earlier);
}

TEST(TableMemoryTest, ShowdownEntryEquality_ComparesEveryField)
{
    const antwika::holdem::ShowdownEntry base{
        .seat = makeSeatId(0),
        .holeCards =
            {static_cast<antwika::holdem::Card>(3),
             static_cast<antwika::holdem::Card>(17)},
        .value = static_cast<antwika::holdem::HandValue>(77)};

    const auto twin = base;
    EXPECT_EQ(base, twin);

    auto elsewhere = base;
    elsewhere.seat = makeSeatId(1);
    EXPECT_NE(base, elsewhere);

    auto redealt = base;
    redealt.holeCards[0] = static_cast<antwika::holdem::Card>(4);
    EXPECT_NE(base, redealt);

    auto weaker = base;
    weaker.value = static_cast<antwika::holdem::HandValue>(76);
    EXPECT_NE(base, weaker);
}
