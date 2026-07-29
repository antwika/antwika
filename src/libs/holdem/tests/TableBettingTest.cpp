#include <gtest/gtest.h>

#include <cstddef>

#include <antwika/holdem/Action.hpp>
#include <antwika/holdem/Blinds.hpp>
#include <antwika/holdem/CardText.hpp>
#include <antwika/holdem/Chips.hpp>
#include <antwika/holdem/IllegalActionError.hpp>
#include <antwika/holdem/SeatId.hpp>
#include <antwika/holdem/Stage.hpp>
#include <antwika/holdem/Table.hpp>

#include <antwika/holdem/fakes/FakeDeck.hpp>

using antwika::holdem::bet;
using antwika::holdem::Blinds;
using antwika::holdem::call;
using antwika::holdem::check;
using antwika::holdem::Chips;
using antwika::holdem::fold;
using antwika::holdem::IllegalActionError;
using antwika::holdem::makeSeatId;
using antwika::holdem::parseCards;
using antwika::holdem::raiseTo;
using antwika::holdem::SeatId;
using antwika::holdem::Stage;
using antwika::holdem::Table;
using antwika::holdem::fakes::FakeDeck;

namespace
{
    constexpr Blinds kBlinds{.small = 5, .big = 10};

    // Two-handed, seat 0 is dealt first and seat 1 gets the button.
    // So seat 0 holds the aces and seat 1 the kings.
    [[nodiscard]] FakeDeck headsUpDeck()
    {
        return FakeDeck(parseCards("Ac Kc Ad Kd 2c 3d 4h 7s 9c"));
    }

    [[nodiscard]] FakeDeck threeHandedDeck()
    {
        return FakeDeck(
            parseCards("Ac Kc Qc Ad Kd Qd 2c 3d 4h 7s 9h"));
    }

    [[nodiscard]] Chips stackOf(const Table &table, std::size_t seat)
    {
        return table.seatAt(makeSeatId(seat)).stack;
    }

    void seatTwo(Table &table, Chips first, Chips second)
    {
        table.seatPlayer(makeSeatId(0), first);
        table.seatPlayer(makeSeatId(1), second);
    }
} // namespace

TEST(TableBettingTest, StartHand_PostsBothBlindsAndMovesTheButton)
{
    Table table(2, kBlinds);
    seatTwo(table, 100, 100);
    auto deck = headsUpDeck();
    table.startHand(deck);

    EXPECT_EQ(table.button(), makeSeatId(1));
    EXPECT_EQ(table.pot(), 15U);
    EXPECT_EQ(stackOf(table, 0), 90U);
    EXPECT_EQ(stackOf(table, 1), 95U);
    EXPECT_EQ(table.handsPlayed(), 1U);
    EXPECT_EQ(table.stage(), Stage::PreFlop);
    EXPECT_EQ(deck.shuffleCount(), 1U);
}

// Heads-up the button posts the small blind.
// So it also has the first decision pre-flop.
TEST(TableBettingTest, StartHand_OpensOnTheButtonWhenHeadsUp)
{
    Table table(2, kBlinds);
    seatTwo(table, 100, 100);
    auto deck = headsUpDeck();
    table.startHand(deck);

    ASSERT_TRUE(table.seatToAct().has_value());
    EXPECT_EQ(*table.seatToAct(), makeSeatId(1));
}

TEST(TableBettingTest, StartHand_DealsTwoHoleCardsToEachPlayer)
{
    Table table(2, kBlinds);
    seatTwo(table, 100, 100);
    auto deck = headsUpDeck();
    table.startHand(deck);

    const auto hole = table.seatAt(makeSeatId(0)).holeCards;
    EXPECT_EQ(antwika::holdem::toString(hole), "Ac Ad");
    const auto other = table.seatAt(makeSeatId(1)).holeCards;
    EXPECT_EQ(antwika::holdem::toString(other), "Kc Kd");
}

TEST(TableBettingTest, ViewFor_ReportsWhatACallCostsAndTheMinimumRaise)
{
    Table table(2, kBlinds);
    seatTwo(table, 100, 100);
    auto deck = headsUpDeck();
    table.startHand(deck);

    const auto view = table.viewFor(makeSeatId(1));
    EXPECT_EQ(view.seat, makeSeatId(1));
    EXPECT_EQ(view.stage, Stage::PreFlop);
    EXPECT_EQ(view.pot, 15U);
    EXPECT_EQ(view.stack, 95U);
    EXPECT_EQ(view.currentBet, 10U);
    EXPECT_EQ(view.toCall, 5U);
    EXPECT_EQ(view.minRaiseTo, 20U);
    EXPECT_EQ(view.maxRaiseTo, 100U);
    EXPECT_EQ(view.playersInHand, 2U);
    EXPECT_EQ(view.blinds, kBlinds);
    EXPECT_TRUE(view.board.empty());
}

TEST(TableBettingTest, Apply_AwardsThePotToTheLastPlayerStandingOnAFold)
{
    Table table(2, kBlinds);
    seatTwo(table, 100, 100);
    auto deck = headsUpDeck();
    table.startHand(deck);

    table.apply(fold());

    EXPECT_FALSE(table.isHandInProgress());
    EXPECT_EQ(stackOf(table, 0), 105U);
    EXPECT_EQ(stackOf(table, 1), 95U);
    EXPECT_EQ(table.lastResult().pot, 15U);
    EXPECT_TRUE(table.lastResult().showdown.empty());
    EXPECT_EQ(table.lastResult().stage, Stage::PreFlop);
}

TEST(TableBettingTest, Apply_GivesTheBigBlindItsOptionAfterALimp)
{
    Table table(2, kBlinds);
    seatTwo(table, 100, 100);
    auto deck = headsUpDeck();
    table.startHand(deck);

    table.apply(call());

    ASSERT_TRUE(table.seatToAct().has_value());
    EXPECT_EQ(*table.seatToAct(), makeSeatId(0));
    EXPECT_EQ(table.viewFor(makeSeatId(0)).toCall, 0U);
    EXPECT_EQ(table.stage(), Stage::PreFlop);
}

TEST(TableBettingTest, Apply_MovesToTheFlopOnceTheBigBlindChecks)
{
    Table table(2, kBlinds);
    seatTwo(table, 100, 100);
    auto deck = headsUpDeck();
    table.startHand(deck);

    table.apply(call());
    table.apply(check());

    EXPECT_EQ(table.stage(), Stage::Flop);
    EXPECT_EQ(table.board().size(), 3U);
    EXPECT_EQ(table.pot(), 20U);
}

TEST(TableBettingTest, Apply_RejectsACheckWithABetToCall)
{
    Table table(2, kBlinds);
    seatTwo(table, 100, 100);
    auto deck = headsUpDeck();
    table.startHand(deck);

    EXPECT_THROW(table.apply(check()), IllegalActionError);
}

TEST(TableBettingTest, Apply_RejectsACallWithNothingToCall)
{
    Table table(2, kBlinds);
    seatTwo(table, 100, 100);
    auto deck = headsUpDeck();
    table.startHand(deck);
    table.apply(call());

    EXPECT_THROW(table.apply(call()), IllegalActionError);
}

TEST(TableBettingTest, Apply_RejectsABetWhileABetIsAlreadyLive)
{
    Table table(2, kBlinds);
    seatTwo(table, 100, 100);
    auto deck = headsUpDeck();
    table.startHand(deck);

    EXPECT_THROW(table.apply(bet(40)), IllegalActionError);
}

TEST(TableBettingTest, Apply_RejectsARaiseWithNoBetToRaise)
{
    Table table(2, kBlinds);
    seatTwo(table, 100, 100);
    auto deck = headsUpDeck();
    table.startHand(deck);
    table.apply(call());
    table.apply(check());

    EXPECT_THROW(table.apply(raiseTo(40)), IllegalActionError);
}

TEST(TableBettingTest, Apply_RejectsARaiseBelowTheMinimumWithChipsToSpare)
{
    Table table(2, kBlinds);
    seatTwo(table, 100, 100);
    auto deck = headsUpDeck();
    table.startHand(deck);

    EXPECT_THROW(table.apply(raiseTo(15)), IllegalActionError);
}

TEST(TableBettingTest, Apply_RejectsARaiseThatDoesNotBeatTheCurrentBet)
{
    Table table(2, kBlinds);
    seatTwo(table, 100, 100);
    auto deck = headsUpDeck();
    table.startHand(deck);

    EXPECT_THROW(table.apply(raiseTo(10)), IllegalActionError);
}

TEST(TableBettingTest, Apply_RejectsStakingMoreThanTheStackHolds)
{
    Table table(2, kBlinds);
    seatTwo(table, 100, 100);
    auto deck = headsUpDeck();
    table.startHand(deck);

    EXPECT_THROW(table.apply(raiseTo(101)), IllegalActionError);
}

TEST(TableBettingTest, Apply_AcceptsAMinimumRaiseAndDemandsAFullReRaise)
{
    Table table(2, kBlinds);
    seatTwo(table, 100, 100);
    auto deck = headsUpDeck();
    table.startHand(deck);

    table.apply(raiseTo(20));

    ASSERT_TRUE(table.seatToAct().has_value());
    EXPECT_EQ(*table.seatToAct(), makeSeatId(0));
    EXPECT_EQ(table.viewFor(makeSeatId(0)).toCall, 10U);
    EXPECT_EQ(table.viewFor(makeSeatId(0)).minRaiseTo, 30U);
}

TEST(TableBettingTest, Apply_ReopensTheBettingForAPlayerWhoAlreadyActed)
{
    Table table(2, kBlinds);
    seatTwo(table, 100, 100);
    auto deck = headsUpDeck();
    table.startHand(deck);

    table.apply(call());
    table.apply(raiseTo(30));

    ASSERT_TRUE(table.seatToAct().has_value());
    EXPECT_EQ(*table.seatToAct(), makeSeatId(1));
    EXPECT_EQ(table.viewFor(makeSeatId(1)).toCall, 20U);
}

// A stack too short for a full raise may still go all-in.
// That hands no fresh turn to anyone already square with the bet.
TEST(TableBettingTest, Apply_AllowsAShortAllInBelowTheMinimumRaise)
{
    Table table(2, kBlinds);
    seatTwo(table, 100, 18);
    auto deck = headsUpDeck();
    table.startHand(deck);

    table.apply(raiseTo(18));

    EXPECT_EQ(stackOf(table, 1), 0U);
    EXPECT_EQ(table.pot(), 28U);
    ASSERT_TRUE(table.seatToAct().has_value());
    EXPECT_EQ(*table.seatToAct(), makeSeatId(0));
}

// A short all-in still has to be called.
// But nobody who already matched the bet gets a fresh chance to raise.
TEST(TableBettingTest, Apply_DoesNotReopenBettingOnAShortAllIn)
{
    Table table(3, kBlinds);
    table.seatPlayer(makeSeatId(0), 100);
    table.seatPlayer(makeSeatId(1), 100);
    table.seatPlayer(makeSeatId(2), 14);
    auto deck = threeHandedDeck();
    table.startHand(deck);

    // The button limps, then the short small blind shoves its whole 14.
    // That is four more than the big blind.
    // A full raise would have needed 20.
    ASSERT_EQ(*table.seatToAct(), makeSeatId(1));
    table.apply(call());
    ASSERT_EQ(*table.seatToAct(), makeSeatId(2));
    table.apply(raiseTo(14));

    // The big blind had not spoken yet, so its full turn survives.
    ASSERT_EQ(*table.seatToAct(), makeSeatId(0));
    EXPECT_TRUE(table.viewFor(makeSeatId(0)).mayRaise);
    table.apply(call());

    // The button had already matched the ten.
    // So it owes the difference and nothing more is on offer.
    ASSERT_EQ(*table.seatToAct(), makeSeatId(1));
    EXPECT_FALSE(table.viewFor(makeSeatId(1)).mayRaise);
    EXPECT_EQ(table.viewFor(makeSeatId(1)).toCall, 4U);
    EXPECT_THROW(table.apply(raiseTo(24)), IllegalActionError);

    table.apply(call());
    EXPECT_EQ(table.stage(), Stage::Flop);
}

TEST(TableBettingTest, Apply_ReopensRaisingAgainAfterAFullRaise)
{
    Table table(3, kBlinds);
    table.seatPlayer(makeSeatId(0), 100);
    table.seatPlayer(makeSeatId(1), 100);
    table.seatPlayer(makeSeatId(2), 14);
    auto deck = threeHandedDeck();
    table.startHand(deck);

    table.apply(call());
    table.apply(raiseTo(14));
    table.apply(raiseTo(40));

    // The big blind's full raise puts raising back on the table.
    // The short all-in had taken that away from the button.
    ASSERT_EQ(*table.seatToAct(), makeSeatId(1));
    EXPECT_TRUE(table.viewFor(makeSeatId(1)).mayRaise);
}

TEST(TableBettingTest, StartHand_OpensOnTheButtonWithThreePlayers)
{
    Table table(3, kBlinds);
    table.seatPlayer(makeSeatId(0), 100);
    table.seatPlayer(makeSeatId(1), 100);
    table.seatPlayer(makeSeatId(2), 100);
    auto deck = threeHandedDeck();
    table.startHand(deck);

    // Button on seat 1, so seat 2 is the small blind and seat 0 the big.
    // That leaves the button first to speak.
    EXPECT_EQ(table.button(), makeSeatId(1));
    EXPECT_EQ(table.seatAt(makeSeatId(2)).roundCommitted, 5U);
    EXPECT_EQ(table.seatAt(makeSeatId(0)).roundCommitted, 10U);
    EXPECT_EQ(*table.seatToAct(), makeSeatId(1));
}

TEST(TableBettingTest, Apply_AsksEachPlayerInTurnClockwise)
{
    Table table(3, kBlinds);
    table.seatPlayer(makeSeatId(0), 100);
    table.seatPlayer(makeSeatId(1), 100);
    table.seatPlayer(makeSeatId(2), 100);
    auto deck = threeHandedDeck();
    table.startHand(deck);

    EXPECT_EQ(*table.seatToAct(), makeSeatId(1));
    table.apply(call());
    EXPECT_EQ(*table.seatToAct(), makeSeatId(2));
    table.apply(call());
    EXPECT_EQ(*table.seatToAct(), makeSeatId(0));
    table.apply(check());

    EXPECT_EQ(table.stage(), Stage::Flop);
    EXPECT_EQ(table.pot(), 30U);
}

// Post-flop the first decision belongs to the small blind.
// Not to the button.
TEST(TableBettingTest, Apply_OpensPostFlopBettingLeftOfTheButton)
{
    Table table(3, kBlinds);
    table.seatPlayer(makeSeatId(0), 100);
    table.seatPlayer(makeSeatId(1), 100);
    table.seatPlayer(makeSeatId(2), 100);
    auto deck = threeHandedDeck();
    table.startHand(deck);
    table.apply(call());
    table.apply(call());
    table.apply(check());

    EXPECT_EQ(*table.seatToAct(), makeSeatId(2));
    EXPECT_EQ(table.viewFor(makeSeatId(2)).minRaiseTo, 10U);
    EXPECT_EQ(table.viewFor(makeSeatId(2)).toCall, 0U);
}

TEST(TableBettingTest, Apply_AcceptsAnOpeningBetOfOneBigBlind)
{
    Table table(3, kBlinds);
    table.seatPlayer(makeSeatId(0), 100);
    table.seatPlayer(makeSeatId(1), 100);
    table.seatPlayer(makeSeatId(2), 100);
    auto deck = threeHandedDeck();
    table.startHand(deck);
    table.apply(call());
    table.apply(call());
    table.apply(check());

    table.apply(bet(10));

    EXPECT_EQ(table.pot(), 40U);
    EXPECT_EQ(*table.seatToAct(), makeSeatId(0));
}

TEST(TableBettingTest, Apply_RejectsAnOpeningBetBelowOneBigBlind)
{
    Table table(3, kBlinds);
    table.seatPlayer(makeSeatId(0), 100);
    table.seatPlayer(makeSeatId(1), 100);
    table.seatPlayer(makeSeatId(2), 100);
    auto deck = threeHandedDeck();
    table.startHand(deck);
    table.apply(call());
    table.apply(call());
    table.apply(check());

    EXPECT_THROW(table.apply(bet(5)), IllegalActionError);
}

// The bet nobody called comes back.
// An uncontested raise costs nothing but the blinds already in.
TEST(TableBettingTest, Apply_ReturnsAnUncalledBetToTheOnlyPlayerLeft)
{
    Table table(2, kBlinds);
    seatTwo(table, 100, 100);
    auto deck = headsUpDeck();
    table.startHand(deck);

    table.apply(raiseTo(40));
    table.apply(fold());

    EXPECT_FALSE(table.isHandInProgress());
    EXPECT_EQ(stackOf(table, 1), 110U);
    EXPECT_EQ(stackOf(table, 0), 90U);
}

TEST(TableBettingTest, StartHand_MovesTheButtonAlongEachHand)
{
    Table table(3, kBlinds);
    table.seatPlayer(makeSeatId(0), 100);
    table.seatPlayer(makeSeatId(1), 100);
    table.seatPlayer(makeSeatId(2), 100);

    auto deck = threeHandedDeck();
    table.startHand(deck);
    EXPECT_EQ(table.button(), makeSeatId(1));
    table.apply(fold());
    table.apply(fold());

    table.startHand(deck);
    EXPECT_EQ(table.button(), makeSeatId(2));
    table.apply(fold());
    table.apply(fold());

    table.startHand(deck);
    EXPECT_EQ(table.button(), makeSeatId(0));
    EXPECT_EQ(table.handsPlayed(), 3U);
}

TEST(TableBettingTest, StartHand_SkipsAStackThatBustedWhenMovingTheButton)
{
    Table table(3, kBlinds);
    table.seatPlayer(makeSeatId(0), 100);
    table.seatPlayer(makeSeatId(1), 100);
    table.seatPlayer(makeSeatId(2), 0);
    auto deck = threeHandedDeck();

    table.startHand(deck);

    // Seat 2 has no chips, so it is neither dealt in nor given the button.
    // The hand plays out heads-up between seats 0 and 1.
    EXPECT_EQ(table.button(), makeSeatId(1));
    EXPECT_FALSE(table.seatAt(makeSeatId(2)).inHand);
    EXPECT_EQ(table.viewFor(makeSeatId(1)).playersInHand, 2U);
}
