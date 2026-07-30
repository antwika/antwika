#include <gtest/gtest.h>

#include <string>
#include <vector>

#include <antwika/holdem/Blinds.hpp>
#include <antwika/holdem/CardText.hpp>
#include <antwika/holdem/SeatId.hpp>
#include <antwika/holdem/Table.hpp>

#include <antwika/holdem/fakes/FakeDeck.hpp>

#include "antwika/poker/BankrollError.hpp"
#include "antwika/poker/BankrollLedger.hpp"
#include "antwika/poker/CashGame.hpp"
#include "antwika/poker/CashGameError.hpp"

using antwika::holdem::Blinds;
using antwika::holdem::makeSeatId;
using antwika::holdem::parseCards;
using antwika::holdem::Table;
using antwika::holdem::fakes::FakeDeck;
using antwika::poker::BankrollError;
using antwika::poker::BankrollLedger;
using antwika::poker::CashGame;
using antwika::poker::CashGameError;

namespace
{
    constexpr Blinds kBlinds{.small = 5, .big = 10};
    constexpr antwika::holdem::Chips kMinBuyIn = 100;

    [[nodiscard]] FakeDeck anyDeck()
    {
        return FakeDeck(parseCards("Ac Kc Ad Kd 2c 3d 4h 7s 9c"));
    }
} // namespace

TEST(CashGameTest, BuyIn_MovesChipsFromTheBankrollOntoASeat)
{
    Table table(3, kBlinds);
    BankrollLedger ledger;
    ledger.deposit("alice", 500);
    CashGame game(table, ledger, kMinBuyIn);

    const auto seat = game.buyIn("alice", 200);

    EXPECT_EQ(seat, makeSeatId(0));
    EXPECT_EQ(table.seatAt(seat).stack, 200U);
    EXPECT_EQ(ledger.balanceOf("alice"), 300U);
    EXPECT_EQ(game.playerAt(seat), "alice");
    EXPECT_EQ(game.seatOf("alice"), seat);
}

// The requirement the whole cash desk exists to enforce.
TEST(CashGameTest, BuyIn_RefusesMoreThanThePlayersOwnBankroll)
{
    Table table(3, kBlinds);
    BankrollLedger ledger;
    ledger.deposit("alice", 150);
    CashGame game(table, ledger, kMinBuyIn);

    EXPECT_THROW(
        static_cast<void>(game.buyIn("alice", 200)), BankrollError);
    EXPECT_EQ(ledger.balanceOf("alice"), 150U);
    EXPECT_FALSE(table.seatAt(makeSeatId(0)).occupied);
}

TEST(CashGameTest, BuyIn_RefusesLessThanTheTableMinimum)
{
    Table table(3, kBlinds);
    BankrollLedger ledger;
    ledger.deposit("alice", 500);
    CashGame game(table, ledger, kMinBuyIn);

    EXPECT_THROW(
        static_cast<void>(game.buyIn("alice", 99)), CashGameError);
    EXPECT_EQ(ledger.balanceOf("alice"), 500U);
}

TEST(CashGameTest, BuyIn_RefusesAFullTable)
{
    Table table(2, kBlinds);
    BankrollLedger ledger;
    ledger.deposit("alice", 500);
    ledger.deposit("bob", 500);
    ledger.deposit("carol", 500);
    CashGame game(table, ledger, kMinBuyIn);
    static_cast<void>(game.buyIn("alice", 200));
    static_cast<void>(game.buyIn("bob", 200));

    EXPECT_THROW(
        static_cast<void>(game.buyIn("carol", 200)), CashGameError);
    EXPECT_EQ(ledger.balanceOf("carol"), 500U);
}

TEST(CashGameTest, BuyIn_TopsUpAPlayerWhoIsAlreadySeated)
{
    Table table(3, kBlinds);
    BankrollLedger ledger;
    ledger.deposit("alice", 500);
    CashGame game(table, ledger, kMinBuyIn);
    const auto seat = game.buyIn("alice", 200);

    EXPECT_EQ(game.buyIn("alice", 150), seat);
    EXPECT_EQ(table.seatAt(seat).stack, 350U);
    EXPECT_EQ(ledger.balanceOf("alice"), 150U);
}

TEST(CashGameTest, BuyIn_RefusesATopUpInTheMiddleOfAHand)
{
    Table table(2, kBlinds);
    BankrollLedger ledger;
    ledger.deposit("alice", 500);
    ledger.deposit("bob", 500);
    CashGame game(table, ledger, kMinBuyIn);
    static_cast<void>(game.buyIn("alice", 200));
    static_cast<void>(game.buyIn("bob", 200));
    auto deck = anyDeck();
    table.startHand(deck);

    EXPECT_THROW(
        static_cast<void>(game.buyIn("alice", 100)), CashGameError);
    EXPECT_EQ(ledger.balanceOf("alice"), 300U);
}

TEST(CashGameTest, CashOut_ReturnsWhateverIsLeftAndFreesTheSeat)
{
    Table table(3, kBlinds);
    BankrollLedger ledger;
    ledger.deposit("alice", 500);
    CashGame game(table, ledger, kMinBuyIn);
    const auto seat = game.buyIn("alice", 200);

    game.cashOut("alice");

    EXPECT_EQ(ledger.balanceOf("alice"), 500U);
    EXPECT_FALSE(table.seatAt(seat).occupied);
    EXPECT_FALSE(game.playerAt(seat).has_value());
    EXPECT_FALSE(game.seatOf("alice").has_value());
}

TEST(CashGameTest, CashOut_RefusesSomebodyWhoIsNotAtTheTable)
{
    Table table(3, kBlinds);
    BankrollLedger ledger;
    CashGame game(table, ledger, kMinBuyIn);

    EXPECT_THROW(game.cashOut("nobody"), CashGameError);
}

TEST(CashGameTest, CashOut_RefusesSomebodyInTheMiddleOfAHand)
{
    Table table(2, kBlinds);
    BankrollLedger ledger;
    ledger.deposit("alice", 500);
    ledger.deposit("bob", 500);
    CashGame game(table, ledger, kMinBuyIn);
    static_cast<void>(game.buyIn("alice", 200));
    static_cast<void>(game.buyIn("bob", 200));
    auto deck = anyDeck();
    table.startHand(deck);

    EXPECT_THROW(game.cashOut("alice"), CashGameError);
}

TEST(CashGameTest, CashOutBustedPlayers_SendsHomeOnlyTheEmptyStacks)
{
    Table table(3, kBlinds);
    BankrollLedger ledger;
    ledger.deposit("alice", 500);
    ledger.deposit("bob", 500);
    CashGame game(table, ledger, kMinBuyIn);
    const auto aliceSeat = game.buyIn("alice", 200);
    static_cast<void>(game.buyIn("bob", 200));

    // Alice loses her stack; the table is between hands.
    table.removePlayer(aliceSeat);
    table.seatPlayer(aliceSeat, 0);

    const std::vector<std::string> expected{"alice"};
    EXPECT_EQ(game.cashOutBustedPlayers(), expected);
    EXPECT_EQ(ledger.balanceOf("alice"), 300U);
    EXPECT_FALSE(game.seatOf("alice").has_value());
    EXPECT_TRUE(game.seatOf("bob").has_value());
}

TEST(CashGameTest, CashOutEveryone_SendsEverySeatedPlayerHome)
{
    Table table(3, kBlinds);
    BankrollLedger ledger;
    ledger.deposit("alice", 500);
    ledger.deposit("bob", 500);
    CashGame game(table, ledger, kMinBuyIn);
    static_cast<void>(game.buyIn("alice", 200));
    static_cast<void>(game.buyIn("bob", 300));

    const std::vector<std::string> expected{"alice", "bob"};
    EXPECT_EQ(game.cashOutEveryone(), expected);
    EXPECT_EQ(ledger.balanceOf("alice"), 500U);
    EXPECT_EQ(ledger.balanceOf("bob"), 500U);
}

// Chips contesting a live pot are nobody's to take back yet.
// A session cut short mid-hand leaves them where they are.
TEST(CashGameTest, CashOutEveryone_LeavesALiveHandAlone)
{
    Table table(2, kBlinds);
    BankrollLedger ledger;
    ledger.deposit("alice", 500);
    ledger.deposit("bob", 500);
    CashGame game(table, ledger, kMinBuyIn);
    static_cast<void>(game.buyIn("alice", 200));
    static_cast<void>(game.buyIn("bob", 200));
    auto deck = anyDeck();
    table.startHand(deck);

    EXPECT_TRUE(game.cashOutEveryone().empty());
    EXPECT_EQ(ledger.balanceOf("alice"), 300U);
    EXPECT_TRUE(game.seatOf("alice").has_value());
}

TEST(CashGameTest, PlayerAt_ReportsNothingForAnEmptySeat)
{
    Table table(3, kBlinds);
    BankrollLedger ledger;
    CashGame game(table, ledger, kMinBuyIn);

    EXPECT_FALSE(game.playerAt(makeSeatId(1)).has_value());
}

TEST(CashGameTest, BuyIn_TakesTheLowestFreeSeat)
{
    Table table(3, kBlinds);
    BankrollLedger ledger;
    ledger.deposit("alice", 500);
    ledger.deposit("bob", 500);
    CashGame game(table, ledger, kMinBuyIn);

    static_cast<void>(game.buyIn("alice", 100));
    EXPECT_EQ(game.buyIn("bob", 100), makeSeatId(1));
}
