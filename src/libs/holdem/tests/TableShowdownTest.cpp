#include <gtest/gtest.h>

#include <vector>

#include <antwika/holdem/Action.hpp>
#include <antwika/holdem/Blinds.hpp>
#include <antwika/holdem/CardText.hpp>
#include <antwika/holdem/HandCategory.hpp>
#include <antwika/holdem/HandValue.hpp>
#include <antwika/holdem/Payout.hpp>
#include <antwika/holdem/SeatId.hpp>
#include <antwika/holdem/Stage.hpp>
#include <antwika/holdem/Table.hpp>

#include <antwika/holdem/fakes/FakeDeck.hpp>

using antwika::holdem::bet;
using antwika::holdem::Blinds;
using antwika::holdem::call;
using antwika::holdem::categoryOf;
using antwika::holdem::check;
using antwika::holdem::HandCategory;
using antwika::holdem::makeSeatId;
using antwika::holdem::parseCards;
using antwika::holdem::Payout;
using antwika::holdem::raiseTo;
using antwika::holdem::Stage;
using antwika::holdem::Table;
using antwika::holdem::fakes::FakeDeck;

namespace
{
    constexpr Blinds kBlinds{.small = 5, .big = 10};

    void seatAll(Table &table, const std::vector<unsigned> &stacks)
    {
        for (std::size_t index = 0; index < stacks.size(); ++index)
        {
            table.seatPlayer(makeSeatId(index), stacks[index]);
        }
    }

    void checkAround(Table &table)
    {
        table.apply(check());
        table.apply(check());
    }
} // namespace

// Cards are dealt one at a time starting left of the button.
// Heads-up that puts seat 0 on the aces and seat 1 on the kings.
TEST(TableShowdownTest, Showdown_PaysThePotToTheStrongestHand)
{
    Table table(2, kBlinds);
    seatAll(table, {100, 100});
    auto deck = FakeDeck(parseCards("Ac Kc Ad Kd 2c 3d 4h 7s 9c"));
    table.startHand(deck);

    table.apply(call());
    table.apply(check());
    checkAround(table);
    checkAround(table);
    checkAround(table);

    const auto &result = table.lastResult();
    EXPECT_EQ(result.stage, Stage::Showdown);
    EXPECT_EQ(result.pot, 20U);
    EXPECT_EQ(result.board.size(), 5U);
    EXPECT_EQ(table.seatAt(makeSeatId(0)).stack, 110U);
    EXPECT_EQ(table.seatAt(makeSeatId(1)).stack, 90U);

    const std::vector<Payout> expected{
        Payout{.seat = makeSeatId(0), .amount = 20},
    };
    EXPECT_EQ(result.payouts, expected);
}

TEST(TableShowdownTest, Showdown_RevealsEveryPlayerStrongestFirst)
{
    Table table(2, kBlinds);
    seatAll(table, {100, 100});
    auto deck = FakeDeck(parseCards("Ac Kc Ad Kd 2c 3d 4h 7s 9c"));
    table.startHand(deck);

    table.apply(call());
    table.apply(check());
    checkAround(table);
    checkAround(table);
    checkAround(table);

    const auto &showdown = table.lastResult().showdown;
    ASSERT_EQ(showdown.size(), 2U);
    EXPECT_EQ(showdown[0].seat, makeSeatId(0));
    EXPECT_EQ(showdown[1].seat, makeSeatId(1));
    EXPECT_GT(showdown[0].value, showdown[1].value);
    EXPECT_EQ(categoryOf(showdown[0].value), HandCategory::OnePair);
    EXPECT_EQ(
        antwika::holdem::toString(showdown[0].holeCards), "Ac Ad");
}

TEST(TableShowdownTest, Showdown_SplitsThePotBetweenIdenticalHands)
{
    Table table(2, kBlinds);
    seatAll(table, {100, 100});
    auto deck = FakeDeck(parseCards("Ac Ad Kd Kc 2c 3h 4s 7d 9c"));
    table.startHand(deck);

    table.apply(call());
    table.apply(check());
    checkAround(table);
    checkAround(table);
    checkAround(table);

    const auto &result = table.lastResult();
    const std::vector<Payout> expected{
        Payout{.seat = makeSeatId(0), .amount = 10},
        Payout{.seat = makeSeatId(1), .amount = 10},
    };
    EXPECT_EQ(result.payouts, expected);
    EXPECT_EQ(table.seatAt(makeSeatId(0)).stack, 100U);
    EXPECT_EQ(table.seatAt(makeSeatId(1)).stack, 100U);
}

TEST(TableShowdownTest, Showdown_LeavesAFoldedPlayerOutOfTheReveal)
{
    Table table(3, kBlinds);
    seatAll(table, {100, 100, 100});
    auto deck =
        FakeDeck(parseCards("Ac Qc Kc Ad Qd Kd 2h 3s 4d 7h 9s"));
    table.startHand(deck);

    // Button folds, then the blinds check the hand down.
    table.apply(antwika::holdem::fold());
    table.apply(call());
    table.apply(check());
    checkAround(table);
    checkAround(table);
    checkAround(table);

    const auto &showdown = table.lastResult().showdown;
    ASSERT_EQ(showdown.size(), 2U);
    EXPECT_NE(showdown[0].seat, makeSeatId(1));
    EXPECT_NE(showdown[1].seat, makeSeatId(1));
}

// A player all-in for less can win only what they covered.
// So the strongest hand does not always take everything.
TEST(TableShowdownTest, Showdown_CapsAShortStackAtTheLayerItCovered)
{
    Table table(3, kBlinds);
    seatAll(table, {100, 100, 20});
    auto deck =
        FakeDeck(parseCards("Ac Qc Kc Ad Qd Kd 2h 3s 4d 7h 9s"));
    table.startHand(deck);

    // Seat 2 holds the aces but only 20 chips.
    // Seat 1 has the kings and seat 0 the queens.
    table.apply(raiseTo(20));
    table.apply(call());
    table.apply(call());
    table.apply(bet(40));
    table.apply(call());
    checkAround(table);
    checkAround(table);

    const auto &result = table.lastResult();
    EXPECT_EQ(result.pot, 140U);

    // The aces take the 60 they covered.
    // The kings take the 80 built after the short stack ran out.
    const std::vector<Payout> expected{
        Payout{.seat = makeSeatId(1), .amount = 80},
        Payout{.seat = makeSeatId(2), .amount = 60},
    };
    EXPECT_EQ(result.payouts, expected);
    EXPECT_EQ(table.seatAt(makeSeatId(0)).stack, 40U);
    EXPECT_EQ(table.seatAt(makeSeatId(1)).stack, 120U);
    EXPECT_EQ(table.seatAt(makeSeatId(2)).stack, 60U);
}
