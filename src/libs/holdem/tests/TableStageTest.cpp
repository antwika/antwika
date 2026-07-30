#include <gtest/gtest.h>

#include <antwika/holdem/Action.hpp>
#include <antwika/holdem/Blinds.hpp>
#include <antwika/holdem/CardText.hpp>
#include <antwika/holdem/SeatId.hpp>
#include <antwika/holdem/Stage.hpp>
#include <antwika/holdem/Table.hpp>

#include <antwika/holdem/fakes/FakeDeck.hpp>

using antwika::holdem::bet;
using antwika::holdem::Blinds;
using antwika::holdem::call;
using antwika::holdem::check;
using antwika::holdem::fold;
using antwika::holdem::makeSeatId;
using antwika::holdem::parseCards;
using antwika::holdem::raiseTo;
using antwika::holdem::Stage;
using antwika::holdem::Table;
using antwika::holdem::fakes::FakeDeck;

namespace
{
    constexpr Blinds kBlinds{.small = 5, .big = 10};

    [[nodiscard]] FakeDeck headsUpDeck()
    {
        return FakeDeck(parseCards("Ac Kc Ad Kd 2c 3d 4h 7s 9c"));
    }

    void checkAround(Table &table)
    {
        table.apply(check());
        table.apply(check());
    }
} // namespace

TEST(TableStageTest, Apply_WalksEveryStageFromPreFlopToShowdown)
{
    Table table(2, kBlinds);
    table.seatPlayer(makeSeatId(0), 100);
    table.seatPlayer(makeSeatId(1), 100);
    auto deck = headsUpDeck();
    table.startHand(deck);

    EXPECT_EQ(table.stage(), Stage::PreFlop);
    EXPECT_EQ(table.board().size(), 0U);

    table.apply(call());
    table.apply(check());
    EXPECT_EQ(table.stage(), Stage::Flop);
    EXPECT_EQ(table.board().size(), 3U);

    checkAround(table);
    EXPECT_EQ(table.stage(), Stage::Turn);
    EXPECT_EQ(table.board().size(), 4U);

    checkAround(table);
    EXPECT_EQ(table.stage(), Stage::River);
    EXPECT_EQ(table.board().size(), 5U);

    checkAround(table);
    EXPECT_EQ(table.stage(), Stage::Showdown);
    EXPECT_EQ(table.board().size(), 5U);
    EXPECT_FALSE(table.isHandInProgress());
}

TEST(TableStageTest, Apply_ClearsTheBettingAtTheStartOfEachStreet)
{
    Table table(2, kBlinds);
    table.seatPlayer(makeSeatId(0), 100);
    table.seatPlayer(makeSeatId(1), 100);
    auto deck = headsUpDeck();
    table.startHand(deck);

    table.apply(call());
    table.apply(check());

    const auto view = table.viewFor(makeSeatId(0));
    EXPECT_EQ(view.currentBet, 0U);
    EXPECT_EQ(view.toCall, 0U);
    EXPECT_EQ(view.minRaiseTo, kBlinds.big);
    EXPECT_EQ(table.seatAt(makeSeatId(0)).roundCommitted, 0U);
}

// With nobody holding chips there is nothing left to decide.
// The remaining streets are dealt in one go instead.
TEST(TableStageTest, Apply_RunsTheBoardOutOnceEveryoneIsAllIn)
{
    Table table(2, kBlinds);
    table.seatPlayer(makeSeatId(0), 100);
    table.seatPlayer(makeSeatId(1), 100);
    auto deck = headsUpDeck();
    table.startHand(deck);

    table.apply(raiseTo(100));
    table.apply(call());

    EXPECT_EQ(table.stage(), Stage::Showdown);
    EXPECT_EQ(table.board().size(), 5U);
    EXPECT_FALSE(table.isHandInProgress());
    EXPECT_FALSE(table.seatToAct().has_value());
}

// Two stacks smaller than the blinds are all-in the moment they post.
// The hand is over before anybody could be asked anything.
TEST(TableStageTest, StartHand_FinishesAtOnceWhenTheBlindsAreAllTheChips)
{
    Table table(2, kBlinds);
    table.seatPlayer(makeSeatId(0), 10);
    table.seatPlayer(makeSeatId(1), 5);
    auto deck = headsUpDeck();

    table.startHand(deck);

    EXPECT_FALSE(table.isHandInProgress());
    EXPECT_EQ(table.stage(), Stage::Showdown);
    EXPECT_EQ(table.board().size(), 5U);

    // Seat 0 holds the aces against the kings, so it takes both layers.
    EXPECT_EQ(table.seatAt(makeSeatId(0)).stack, 15U);
    EXPECT_EQ(table.seatAt(makeSeatId(1)).stack, 0U);
}

TEST(TableStageTest, Apply_LeavesTheStageAloneWhenAFoldEndsTheHandEarly)
{
    Table table(2, kBlinds);
    table.seatPlayer(makeSeatId(0), 100);
    table.seatPlayer(makeSeatId(1), 100);
    auto deck = headsUpDeck();
    table.startHand(deck);

    table.apply(call());
    table.apply(check());
    table.apply(bet(20));
    table.apply(fold());

    EXPECT_EQ(table.stage(), Stage::Flop);
    EXPECT_FALSE(table.isHandInProgress());
    EXPECT_EQ(table.lastResult().stage, Stage::Flop);
    EXPECT_TRUE(table.lastResult().showdown.empty());
}
