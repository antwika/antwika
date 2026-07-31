#include <gtest/gtest.h>

#include <vector>

#include <antwika/holdem/Action.hpp>
#include <antwika/holdem/ActionType.hpp>
#include <antwika/holdem/Blinds.hpp>
#include <antwika/holdem/Card.hpp>
#include <antwika/holdem/CardText.hpp>
#include <antwika/holdem/IllegalActionError.hpp>
#include <antwika/holdem/Table.hpp>
#include <antwika/holdem/TableStateError.hpp>

#include <antwika/holdem/fakes/FakeDeck.hpp>

using antwika::holdem::Blinds;
using antwika::holdem::check;
using antwika::holdem::makeSeatId;
using antwika::holdem::parseCards;
using antwika::holdem::Table;
using antwika::holdem::TableStateError;
using antwika::holdem::fakes::FakeDeck;

namespace
{
    constexpr Blinds kBlinds{.small = 5, .big = 10};

    [[nodiscard]] FakeDeck twoPlayerDeck()
    {
        return FakeDeck(parseCards("Ac Kc Ad Kd 2c 3c 4c 5c 6c"));
    }

    [[nodiscard]] FakeDeck threePlayerDeck()
    {
        return FakeDeck(
            parseCards("Ac Kc Qc Ad Kd Qd 2c 3d 4h 7s 9h"));
    }
} // namespace

TEST(TableSeatingTest, Construct_RejectsATableWithTooFewSeats)
{
    EXPECT_THROW(Table(1, kBlinds), TableStateError);
}

TEST(TableSeatingTest, Construct_RejectsATableWithTooManySeats)
{
    EXPECT_THROW(Table(10, kBlinds), TableStateError);
}

TEST(TableSeatingTest, Construct_LeavesEverySeatEmpty)
{
    Table table(6, kBlinds);

    EXPECT_EQ(table.seatCount(), 6U);
    EXPECT_EQ(table.blinds(), kBlinds);
    EXPECT_EQ(table.pot(), 0U);
    EXPECT_FALSE(table.isHandInProgress());
    EXPECT_EQ(table.handsPlayed(), 0U);
    EXPECT_TRUE(table.board().empty());
    for (std::size_t index = 0; index < table.seatCount(); ++index)
    {
        EXPECT_FALSE(table.seatAt(makeSeatId(index)).occupied);
    }
}

TEST(TableSeatingTest, SeatPlayer_GivesThatSeatItsStack)
{
    Table table(3, kBlinds);
    table.seatPlayer(makeSeatId(2), 500);

    const auto &seat = table.seatAt(makeSeatId(2));
    EXPECT_TRUE(seat.occupied);
    EXPECT_EQ(seat.stack, 500U);
}

TEST(TableSeatingTest, SeatPlayer_RefusesASeatThatIsTaken)
{
    Table table(3, kBlinds);
    table.seatPlayer(makeSeatId(1), 500);

    EXPECT_THROW(table.seatPlayer(makeSeatId(1), 200), TableStateError);
}

TEST(TableSeatingTest, SeatPlayer_RefusesASeatTheTableDoesNotHave)
{
    Table table(3, kBlinds);

    EXPECT_THROW(table.seatPlayer(makeSeatId(3), 500), TableStateError);
}

TEST(TableSeatingTest, SeatAt_RefusesASeatTheTableDoesNotHave)
{
    const Table table(3, kBlinds);

    EXPECT_THROW(
        static_cast<void>(table.seatAt(makeSeatId(9))), TableStateError);
}

TEST(TableSeatingTest, ViewFor_RefusesASeatTheTableDoesNotHave)
{
    const Table table(3, kBlinds);

    EXPECT_THROW(
        static_cast<void>(table.viewFor(makeSeatId(9))), TableStateError);
}

TEST(TableSeatingTest, FirstFreeSeat_FindsTheLowestEmptySeat)
{
    Table table(3, kBlinds);
    table.seatPlayer(makeSeatId(0), 100);

    ASSERT_TRUE(table.firstFreeSeat().has_value());
    EXPECT_EQ(*table.firstFreeSeat(), makeSeatId(1));
}

TEST(TableSeatingTest, FirstFreeSeat_FindsNothingAtAFullTable)
{
    Table table(2, kBlinds);
    table.seatPlayer(makeSeatId(0), 100);
    table.seatPlayer(makeSeatId(1), 100);

    EXPECT_FALSE(table.firstFreeSeat().has_value());
}

TEST(TableSeatingTest, RemovePlayer_EmptiesTheSeat)
{
    Table table(3, kBlinds);
    table.seatPlayer(makeSeatId(1), 500);
    table.removePlayer(makeSeatId(1));

    EXPECT_FALSE(table.seatAt(makeSeatId(1)).occupied);
    EXPECT_EQ(table.seatAt(makeSeatId(1)).stack, 0U);
}

TEST(TableSeatingTest, RemovePlayer_RefusesASeatTheTableDoesNotHave)
{
    Table table(3, kBlinds);

    EXPECT_THROW(table.removePlayer(makeSeatId(7)), TableStateError);
}

TEST(TableSeatingTest, RemovePlayer_RefusesToPullAPlayerOutOfALiveHand)
{
    Table table(2, kBlinds);
    table.seatPlayer(makeSeatId(0), 100);
    table.seatPlayer(makeSeatId(1), 100);
    auto deck = twoPlayerDeck();
    table.startHand(deck);

    EXPECT_THROW(table.removePlayer(makeSeatId(0)), TableStateError);
}

// Folding gives up the cards, not the chips already in the middle.
// Emptying the seat drops that stake from what finishHand() sees.
// The payouts would then fall short by exactly it.
// Clearing the pot afterwards would discard the difference for good.
TEST(TableSeatingTest, RemovePlayer_RefusesAFolderWhoseChipsAreStillIn)
{
    Table table(3, kBlinds);
    table.seatPlayer(makeSeatId(0), 100);
    table.seatPlayer(makeSeatId(1), 100);
    table.seatPlayer(makeSeatId(2), 100);
    auto deck = threePlayerDeck();
    table.startHand(deck);

    // Seat 1 has the button and limps, then the small blind folds.
    table.apply(antwika::holdem::call());
    table.apply(antwika::holdem::fold());

    ASSERT_FALSE(table.seatAt(makeSeatId(2)).inHand);
    ASSERT_EQ(table.seatAt(makeSeatId(2)).committed, 5U);
    EXPECT_THROW(table.removePlayer(makeSeatId(2)), TableStateError);
}

// A seat that never put a chip in owes the pot nothing.
// Sitting down mid-hand and leaving again takes nothing with it.
TEST(TableSeatingTest, RemovePlayer_LetsASeatWithNothingInTheMiddleLeave)
{
    Table table(3, kBlinds);
    table.seatPlayer(makeSeatId(0), 100);
    table.seatPlayer(makeSeatId(1), 100);
    auto deck = threePlayerDeck();
    table.startHand(deck);
    table.seatPlayer(makeSeatId(2), 100);

    table.removePlayer(makeSeatId(2));

    EXPECT_FALSE(table.seatAt(makeSeatId(2)).occupied);
}

// Once the hand is paid out the stake is history rather than a claim.
// A seat goes on reporting what it committed until the next deal.
// So the refusal has to turn on the hand being live.
// Turning on that figure alone would strand everyone at the table.
TEST(TableSeatingTest, RemovePlayer_LetsAFolderLeaveOnceTheHandIsPaidOut)
{
    Table table(3, kBlinds);
    table.seatPlayer(makeSeatId(0), 100);
    table.seatPlayer(makeSeatId(1), 100);
    table.seatPlayer(makeSeatId(2), 100);
    auto deck = threePlayerDeck();
    table.startHand(deck);
    table.apply(antwika::holdem::fold());
    table.apply(antwika::holdem::fold());

    ASSERT_FALSE(table.isHandInProgress());
    ASSERT_EQ(table.seatAt(makeSeatId(2)).committed, 5U);
    table.removePlayer(makeSeatId(2));

    EXPECT_FALSE(table.seatAt(makeSeatId(2)).occupied);
}

TEST(TableSeatingTest, AddChips_TopsUpAStackBetweenHands)
{
    Table table(3, kBlinds);
    table.seatPlayer(makeSeatId(0), 100);
    table.addChips(makeSeatId(0), 50);

    EXPECT_EQ(table.seatAt(makeSeatId(0)).stack, 150U);
}

TEST(TableSeatingTest, AddChips_RefusesAnEmptySeat)
{
    Table table(3, kBlinds);

    EXPECT_THROW(table.addChips(makeSeatId(0), 50), TableStateError);
}

TEST(TableSeatingTest, AddChips_RefusesASeatTheTableDoesNotHave)
{
    Table table(3, kBlinds);

    EXPECT_THROW(table.addChips(makeSeatId(8), 50), TableStateError);
}

TEST(TableSeatingTest, AddChips_RefusesAStackThatIsInAHand)
{
    Table table(2, kBlinds);
    table.seatPlayer(makeSeatId(0), 100);
    table.seatPlayer(makeSeatId(1), 100);
    auto deck = twoPlayerDeck();
    table.startHand(deck);

    EXPECT_THROW(table.addChips(makeSeatId(0), 50), TableStateError);
}

TEST(TableSeatingTest, CanStartHand_NeedsTwoPlayersHoldingChips)
{
    Table table(3, kBlinds);
    EXPECT_FALSE(table.canStartHand());

    table.seatPlayer(makeSeatId(0), 100);
    EXPECT_FALSE(table.canStartHand());

    table.seatPlayer(makeSeatId(1), 0);
    EXPECT_FALSE(table.canStartHand());

    table.addChips(makeSeatId(1), 100);
    EXPECT_TRUE(table.canStartHand());
}

TEST(TableSeatingTest, CanStartHand_IsFalseWhileAHandIsBeingPlayed)
{
    Table table(2, kBlinds);
    table.seatPlayer(makeSeatId(0), 100);
    table.seatPlayer(makeSeatId(1), 100);
    auto deck = twoPlayerDeck();
    table.startHand(deck);

    EXPECT_FALSE(table.canStartHand());
}

TEST(TableSeatingTest, StartHand_RefusesToDealWithoutTwoFundedPlayers)
{
    Table table(2, kBlinds);
    table.seatPlayer(makeSeatId(0), 100);
    auto deck = twoPlayerDeck();

    EXPECT_THROW(table.startHand(deck), TableStateError);
}

TEST(TableSeatingTest, StartHand_RefusesToDealOverALiveHand)
{
    Table table(2, kBlinds);
    table.seatPlayer(makeSeatId(0), 100);
    table.seatPlayer(makeSeatId(1), 100);
    auto deck = twoPlayerDeck();
    table.startHand(deck);

    EXPECT_THROW(table.startHand(deck), TableStateError);
}

TEST(TableSeatingTest, SeatPlayer_IsAllowedMidHandAndSitsOutUntilTheNext)
{
    Table table(3, kBlinds);
    table.seatPlayer(makeSeatId(0), 100);
    table.seatPlayer(makeSeatId(1), 100);
    auto deck = FakeDeck(parseCards("Ac Kc Ad Kd 2c 3c 4c 5c 6c"));
    table.startHand(deck);

    table.seatPlayer(makeSeatId(2), 100);

    EXPECT_TRUE(table.seatAt(makeSeatId(2)).occupied);
    EXPECT_FALSE(table.seatAt(makeSeatId(2)).inHand);
}

TEST(TableSeatingTest, Apply_RefusesAnActionWhenNobodyIsWaiting)
{
    Table table(2, kBlinds);

    EXPECT_THROW(table.apply(check()), TableStateError);
}

TEST(TableSeatingTest, LastResult_RefusesToReportAHandThatHasNotFinished)
{
    const Table table(2, kBlinds);

    EXPECT_THROW(static_cast<void>(table.lastResult()), TableStateError);
}

// A value that names no action at all is a rule violation, not a no-op:
// a table that quietly ignored it would silently skip somebody's turn.
TEST(TableSeatingTest, Apply_RejectsAnActionTypeThatIsNotOneOfTheFive)
{
    Table table(2, kBlinds);
    table.seatPlayer(makeSeatId(0), 100);
    table.seatPlayer(makeSeatId(1), 100);
    auto deck = twoPlayerDeck();
    table.startHand(deck);

    const antwika::holdem::Action nonsense{
        .type = static_cast<antwika::holdem::ActionType>(42),
        .amount = 0,
    };
    EXPECT_THROW(
        table.apply(nonsense), antwika::holdem::IllegalActionError);
}

// Narrating a session means reading a finished hand's result.
// The next one is already being dealt by then.
TEST(TableSeatingTest, LastResult_KeepsTheLastHandWhileTheNextOneRuns)
{
    Table table(2, kBlinds);
    table.seatPlayer(makeSeatId(0), 100);
    table.seatPlayer(makeSeatId(1), 100);
    auto deck = twoPlayerDeck();
    table.startHand(deck);
    table.apply(antwika::holdem::fold());
    const auto firstPot = table.lastResult().pot;

    table.startHand(deck);

    EXPECT_TRUE(table.isHandInProgress());
    EXPECT_EQ(table.lastResult().pot, firstPot);
}
