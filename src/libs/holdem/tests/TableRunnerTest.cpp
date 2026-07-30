#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <functional>
#include <vector>

#include <antwika/holdem/Action.hpp>
#include <antwika/holdem/Blinds.hpp>
#include <antwika/holdem/CardText.hpp>
#include <antwika/holdem/IAgent.hpp>
#include <antwika/holdem/SeatId.hpp>
#include <antwika/holdem/Stage.hpp>
#include <antwika/holdem/StepOutcome.hpp>
#include <antwika/holdem/Table.hpp>
#include <antwika/holdem/TableRunner.hpp>
#include <antwika/holdem/TableStateError.hpp>

#include <antwika/holdem/fakes/FakeDeck.hpp>
#include <antwika/holdem/mocks/MockAgent.hpp>

using antwika::holdem::Blinds;
using antwika::holdem::call;
using antwika::holdem::check;
using antwika::holdem::fold;
using antwika::holdem::IAgent;
using antwika::holdem::makeSeatId;
using antwika::holdem::parseCards;
using antwika::holdem::raiseTo;
using antwika::holdem::StepKind;
using antwika::holdem::Table;
using antwika::holdem::TableRunner;
using antwika::holdem::TableStateError;
using antwika::holdem::fakes::FakeDeck;
using antwika::holdem::mocks::MockAgent;
using testing::NiceMock;
using testing::Return;

namespace
{
    constexpr Blinds kBlinds{.small = 5, .big = 10};

    [[nodiscard]] FakeDeck headsUpDeck()
    {
        return FakeDeck(parseCards("Ac Kc Ad Kd 2c 3d 4h 7s 9c"));
    }

    [[nodiscard]] std::vector<std::reference_wrapper<IAgent>> pair(
        IAgent &first, IAgent &second)
    {
        return {first, second};
    }
} // namespace

TEST(TableRunnerTest, Construct_RefusesAnAgentListThatMissesASeat)
{
    Table table(3, kBlinds);
    auto deck = headsUpDeck();
    NiceMock<MockAgent> first;
    NiceMock<MockAgent> second;

    EXPECT_THROW(
        TableRunner(table, deck, pair(first, second)), TableStateError);
}

TEST(TableRunnerTest, Step_ReportsAnIdleTableWhenNoHandCanBeDealt)
{
    Table table(2, kBlinds);
    auto deck = headsUpDeck();
    NiceMock<MockAgent> first;
    NiceMock<MockAgent> second;
    TableRunner runner(table, deck, pair(first, second));

    const auto outcome = runner.step();

    EXPECT_EQ(outcome.kind, StepKind::TableIdle);
    EXPECT_EQ(table.handsPlayed(), 0U);
}

TEST(TableRunnerTest, Step_DealsAHandBeforeAskingAnybodyAnything)
{
    Table table(2, kBlinds);
    table.seatPlayer(makeSeatId(0), 100);
    table.seatPlayer(makeSeatId(1), 100);
    auto deck = headsUpDeck();
    NiceMock<MockAgent> first;
    NiceMock<MockAgent> second;
    EXPECT_CALL(first, act).Times(0);
    EXPECT_CALL(second, act).Times(0);
    TableRunner runner(table, deck, pair(first, second));

    const auto outcome = runner.step();

    EXPECT_EQ(outcome.kind, StepKind::HandStarted);
    EXPECT_EQ(outcome.stage, antwika::holdem::Stage::PreFlop);
    EXPECT_EQ(table.handsPlayed(), 1U);
}

TEST(TableRunnerTest, Step_AsksOnlyTheAgentWhoseTurnItIs)
{
    Table table(2, kBlinds);
    table.seatPlayer(makeSeatId(0), 100);
    table.seatPlayer(makeSeatId(1), 100);
    auto deck = headsUpDeck();
    NiceMock<MockAgent> first;
    NiceMock<MockAgent> second;
    TableRunner runner(table, deck, pair(first, second));
    static_cast<void>(runner.step());

    // The button posts the small blind heads-up, so seat 1 speaks first.
    EXPECT_CALL(first, act).Times(0);
    EXPECT_CALL(second, act).WillOnce(Return(call()));

    const auto outcome = runner.step();

    EXPECT_EQ(outcome.kind, StepKind::Acted);
    EXPECT_EQ(outcome.seat, makeSeatId(1));
    EXPECT_EQ(outcome.action, call());
    EXPECT_FALSE(outcome.stageAdvanced);
}

TEST(TableRunnerTest, Step_HandsTheAgentTheViewOfItsOwnSeat)
{
    Table table(2, kBlinds);
    table.seatPlayer(makeSeatId(0), 100);
    table.seatPlayer(makeSeatId(1), 100);
    auto deck = headsUpDeck();
    NiceMock<MockAgent> first;
    NiceMock<MockAgent> second;
    TableRunner runner(table, deck, pair(first, second));
    static_cast<void>(runner.step());

    EXPECT_CALL(second, act)
        .WillOnce(
            [](const antwika::holdem::TableView &view)
            {
                EXPECT_EQ(view.seat, makeSeatId(1));
                EXPECT_EQ(view.toCall, 5U);
                EXPECT_EQ(
                    antwika::holdem::toString(view.holeCards), "Kc Kd");
                return call();
            });

    static_cast<void>(runner.step());
}

TEST(TableRunnerTest, Step_FlagsTheStepThatClosesABettingRound)
{
    Table table(2, kBlinds);
    table.seatPlayer(makeSeatId(0), 100);
    table.seatPlayer(makeSeatId(1), 100);
    auto deck = headsUpDeck();
    NiceMock<MockAgent> first;
    NiceMock<MockAgent> second;
    ON_CALL(first, act).WillByDefault(Return(check()));
    ON_CALL(second, act).WillByDefault(Return(call()));
    TableRunner runner(table, deck, pair(first, second));

    static_cast<void>(runner.step());
    const auto limp = runner.step();
    const auto closed = runner.step();

    EXPECT_FALSE(limp.stageAdvanced);
    EXPECT_TRUE(closed.stageAdvanced);
    EXPECT_EQ(closed.stage, antwika::holdem::Stage::Flop);
}

TEST(TableRunnerTest, Step_ReportsTheActionThatEndsTheHand)
{
    Table table(2, kBlinds);
    table.seatPlayer(makeSeatId(0), 100);
    table.seatPlayer(makeSeatId(1), 100);
    auto deck = headsUpDeck();
    NiceMock<MockAgent> first;
    NiceMock<MockAgent> second;
    ON_CALL(second, act).WillByDefault(Return(fold()));
    TableRunner runner(table, deck, pair(first, second));

    static_cast<void>(runner.step());
    const auto outcome = runner.step();

    EXPECT_EQ(outcome.kind, StepKind::HandCompleted);
    EXPECT_EQ(outcome.seat, makeSeatId(1));
    EXPECT_EQ(outcome.action, fold());
    EXPECT_FALSE(table.isHandInProgress());
}

// A call names no amount, because the table decides what it costs.
// So the step is where a caller finds out what it cost.
TEST(TableRunnerTest, Step_ReportsWhatACallCostAndWhatItAnswered)
{
    Table table(2, kBlinds);
    table.seatPlayer(makeSeatId(0), 100);
    table.seatPlayer(makeSeatId(1), 100);
    auto deck = headsUpDeck();
    NiceMock<MockAgent> first;
    NiceMock<MockAgent> second;
    ON_CALL(second, act).WillByDefault(Return(call()));
    TableRunner runner(table, deck, pair(first, second));

    static_cast<void>(runner.step());
    const auto outcome = runner.step();

    EXPECT_EQ(outcome.staked, 5U);
    EXPECT_EQ(outcome.betBefore, 10U);
    EXPECT_FALSE(outcome.allIn);
}

TEST(TableRunnerTest, Step_ReportsTheBetARaiseWasMeasuredAgainst)
{
    Table table(2, kBlinds);
    table.seatPlayer(makeSeatId(0), 100);
    table.seatPlayer(makeSeatId(1), 100);
    auto deck = headsUpDeck();
    NiceMock<MockAgent> first;
    NiceMock<MockAgent> second;
    ON_CALL(second, act).WillByDefault(Return(raiseTo(30)));
    TableRunner runner(table, deck, pair(first, second));

    static_cast<void>(runner.step());
    const auto outcome = runner.step();

    EXPECT_EQ(outcome.staked, 25U);
    EXPECT_EQ(outcome.betBefore, 10U);
    EXPECT_FALSE(outcome.allIn);
}

TEST(TableRunnerTest, Step_FlagsTheActionThatUsedUpASeatsLastChip)
{
    Table table(2, kBlinds);
    table.seatPlayer(makeSeatId(0), 100);
    table.seatPlayer(makeSeatId(1), 40);
    auto deck = headsUpDeck();
    NiceMock<MockAgent> first;
    NiceMock<MockAgent> second;
    ON_CALL(second, act).WillByDefault(Return(raiseTo(40)));
    TableRunner runner(table, deck, pair(first, second));

    static_cast<void>(runner.step());
    const auto outcome = runner.step();

    EXPECT_EQ(outcome.staked, 35U);
    EXPECT_TRUE(outcome.allIn);
}

TEST(TableRunnerTest, Step_DealsTheNextHandOnceTheLastOneIsPaidOut)
{
    Table table(2, kBlinds);
    table.seatPlayer(makeSeatId(0), 100);
    table.seatPlayer(makeSeatId(1), 100);
    auto deck = headsUpDeck();
    NiceMock<MockAgent> first;
    NiceMock<MockAgent> second;
    ON_CALL(second, act).WillByDefault(Return(fold()));
    TableRunner runner(table, deck, pair(first, second));

    static_cast<void>(runner.step());
    static_cast<void>(runner.step());
    const auto outcome = runner.step();

    EXPECT_EQ(outcome.kind, StepKind::HandStarted);
    EXPECT_EQ(table.handsPlayed(), 2U);
    EXPECT_EQ(deck.shuffleCount(), 2U);
}

// Both blinds are the whole stack here.
// So the deal itself settles the hand and nobody is asked to act.
TEST(TableRunnerTest, Step_ReportsAHandThatFinishedOnTheDealItself)
{
    Table table(2, kBlinds);
    table.seatPlayer(makeSeatId(0), 10);
    table.seatPlayer(makeSeatId(1), 5);
    auto deck = headsUpDeck();
    NiceMock<MockAgent> first;
    NiceMock<MockAgent> second;
    EXPECT_CALL(first, act).Times(0);
    EXPECT_CALL(second, act).Times(0);
    TableRunner runner(table, deck, pair(first, second));

    const auto outcome = runner.step();

    EXPECT_EQ(outcome.kind, StepKind::HandCompleted);
    EXPECT_FALSE(table.isHandInProgress());
}

TEST(TableRunnerTest, Step_GoesIdleOnceOnlyOnePlayerHasChipsLeft)
{
    Table table(2, kBlinds);
    table.seatPlayer(makeSeatId(0), 10);
    table.seatPlayer(makeSeatId(1), 5);
    auto deck = headsUpDeck();
    NiceMock<MockAgent> first;
    NiceMock<MockAgent> second;
    TableRunner runner(table, deck, pair(first, second));

    static_cast<void>(runner.step());
    const auto outcome = runner.step();

    EXPECT_EQ(outcome.kind, StepKind::TableIdle);
    EXPECT_EQ(table.handsPlayed(), 1U);
}
