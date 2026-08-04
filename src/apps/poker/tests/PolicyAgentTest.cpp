#include <gtest/gtest.h>

#include <cstddef>
#include <functional>
#include <string_view>
#include <vector>

#include <antwika/holdem/Action.hpp>
#include <antwika/holdem/ActionType.hpp>
#include <antwika/holdem/Blinds.hpp>
#include <antwika/holdem/Card.hpp>
#include <antwika/holdem/CardText.hpp>
#include <antwika/holdem/Deck.hpp>
#include <antwika/holdem/IAgent.hpp>
#include <antwika/holdem/SeatId.hpp>
#include <antwika/holdem/Stage.hpp>
#include <antwika/holdem/StepOutcome.hpp>
#include <antwika/holdem/Table.hpp>
#include <antwika/holdem/TableRunner.hpp>
#include <antwika/holdem/TableView.hpp>
#include <antwika/rng/SplitMix64Rng.hpp>

#include "antwika/poker/AgentStyle.hpp"
#include "antwika/poker/PolicyAgent.hpp"
#include "antwika/poker/RoomConfig.hpp"

using antwika::holdem::ActionType;
using antwika::holdem::Blinds;
using antwika::holdem::Card;
using antwika::holdem::Deck;
using antwika::holdem::IAgent;
using antwika::holdem::makeSeatId;
using antwika::holdem::parseCards;
using antwika::holdem::Stage;
using antwika::holdem::StepKind;
using antwika::holdem::Table;
using antwika::holdem::TableRunner;
using antwika::holdem::TableView;
using antwika::poker::AgentStyle;
using antwika::poker::handStrength;
using antwika::poker::PolicyAgent;
using antwika::poker::kDefaultHandStrengths;
using antwika::poker::kDefaultThresholds;
using antwika::rng::SplitMix64Rng;

namespace
{
    [[nodiscard]] TableView viewWith(
        std::string_view hole, std::string_view board)
    {
        const auto holeCards = parseCards(hole);
        TableView view;
        view.holeCards[0] = holeCards[0];
        view.holeCards[1] = holeCards[1];
        view.board = parseCards(board);
        view.stage = view.board.empty() ? Stage::PreFlop : Stage::Flop;
        view.stack = 1000;
        view.maxRaiseTo = 1000;
        view.minRaiseTo = 20;
        view.pot = 30;
        view.playersInHand = 2;
        view.blinds = Blinds{.small = 5, .big = 10};
        return view;
    }
} // namespace

TEST(PolicyAgentTest, HandStrength_RatesAPocketPairAboveTwoBlanks)
{
    EXPECT_GT(
        handStrength(viewWith("Ac Ad", ""), kDefaultHandStrengths),
        handStrength(viewWith("7c 2d", ""), kDefaultHandStrengths));
}

TEST(PolicyAgentTest, HandStrength_RatesHigherCardsHigher)
{
    EXPECT_GT(
        handStrength(viewWith("Ac 8d", ""), kDefaultHandStrengths),
        handStrength(viewWith("Tc 8d", ""), kDefaultHandStrengths));
}

TEST(PolicyAgentTest, HandStrength_RewardsBeingSuited)
{
    EXPECT_GT(
        handStrength(viewWith("Kc 8c", ""), kDefaultHandStrengths),
        handStrength(viewWith("Kc 8d", ""), kDefaultHandStrengths));
}

TEST(PolicyAgentTest, HandStrength_RewardsBeingConnected)
{
    EXPECT_GT(
        handStrength(viewWith("Kd Qc", ""), kDefaultHandStrengths),
        handStrength(viewWith("Kd Jc", ""), kDefaultHandStrengths));
}

TEST(PolicyAgentTest, HandStrength_PenalisesAWideGap)
{
    EXPECT_GT(
        handStrength(viewWith("Kd Jc", ""), kDefaultHandStrengths),
        handStrength(viewWith("Kd 7c", ""), kDefaultHandStrengths));
}

// From the flop on the agent stops guessing and asks the evaluator.
TEST(PolicyAgentTest, HandStrength_RanksMadeHandsByCategory)
{
    const auto highCard = handStrength(
        viewWith("Ac Kd", "7h 4s 2c"), kDefaultHandStrengths);
    const auto pair = handStrength(
        viewWith("Ac Ad", "7h 4s 2c"), kDefaultHandStrengths);
    const auto twoPair = handStrength(
        viewWith("Ac 7d", "7h Ah 2c"), kDefaultHandStrengths);
    const auto trips = handStrength(
        viewWith("Ac Ad", "Ah 4s 2c"), kDefaultHandStrengths);
    const auto straight = handStrength(
        viewWith("6c 5d", "7h 8s 9c"), kDefaultHandStrengths);
    const auto flush = handStrength(
        viewWith("Ac 8c", "7c 4c 2c"), kDefaultHandStrengths);
    const auto quads = handStrength(
        viewWith("Ac Ad", "Ah As 2c"), kDefaultHandStrengths);
    const auto straightFlush =
        handStrength(viewWith("6c 5c", "7c 8c 9c"), kDefaultHandStrengths);

    EXPECT_LT(highCard, pair);
    EXPECT_LT(pair, twoPair);
    EXPECT_LT(twoPair, trips);
    EXPECT_LT(trips, straight);
    EXPECT_LT(straight, flush);
    EXPECT_LT(flush, quads);
    EXPECT_LT(quads, straightFlush);
    EXPECT_EQ(straightFlush, 100U);
}

TEST(PolicyAgentTest, HandStrength_ScoresAFullHouseBetweenFlushAndQuads)
{
    const auto flush = handStrength(
        viewWith("Ac 8c", "7c 4c 2c"), kDefaultHandStrengths);
    const auto fullHouse = handStrength(
        viewWith("Ac Ad", "Ah 4s 4c"), kDefaultHandStrengths);
    const auto quads = handStrength(
        viewWith("Ac Ad", "Ah As 2c"), kDefaultHandStrengths);

    EXPECT_LT(flush, fullHouse);
    EXPECT_LT(fullHouse, quads);
}

TEST(PolicyAgentTest, PlayingStyle_ReportsWhatItWasBuiltWith)
{
    const PolicyAgent agent(AgentStyle::Tight, kDefaultHandStrengths,
            kDefaultThresholds);

    EXPECT_EQ(agent.playingStyle(), AgentStyle::Tight);
}

TEST(PolicyAgentTest, Act_ChecksRatherThanFoldingWhenItIsFree)
{
    PolicyAgent agent(AgentStyle::Tight, kDefaultHandStrengths,
            kDefaultThresholds);
    auto view = viewWith("7c 2d", "");
    view.toCall = 0;
    view.currentBet = 0;

    EXPECT_EQ(agent.act(view).type, ActionType::Check);
}

TEST(PolicyAgentTest, Act_FoldsAWeakHandFacingABet)
{
    PolicyAgent agent(AgentStyle::Tight, kDefaultHandStrengths,
            kDefaultThresholds);
    auto view = viewWith("7c 2d", "");
    view.toCall = 40;
    view.currentBet = 40;

    EXPECT_EQ(agent.act(view).type, ActionType::Fold);
}

TEST(PolicyAgentTest, Act_CallsAMiddlingHandFacingABet)
{
    PolicyAgent agent(AgentStyle::Balanced, kDefaultHandStrengths,
            kDefaultThresholds);
    auto view = viewWith("Ac 9d", "");
    view.toCall = 40;
    view.currentBet = 40;

    EXPECT_EQ(agent.act(view).type, ActionType::Call);
}

TEST(PolicyAgentTest, Act_RaisesAStrongHandFacingABet)
{
    PolicyAgent agent(AgentStyle::Balanced, kDefaultHandStrengths,
            kDefaultThresholds);
    auto view = viewWith("Ac Ad", "");
    view.toCall = 40;
    view.currentBet = 40;
    view.minRaiseTo = 80;

    const auto action = agent.act(view);
    EXPECT_EQ(action.type, ActionType::Raise);
    EXPECT_GE(action.amount, view.minRaiseTo);
    EXPECT_LE(action.amount, view.maxRaiseTo);
}

TEST(PolicyAgentTest, Act_OpensWithABetRatherThanARaiseWhenNothingIsLive)
{
    PolicyAgent agent(AgentStyle::Balanced, kDefaultHandStrengths,
            kDefaultThresholds);
    auto view = viewWith("Ac Ad", "");
    view.toCall = 0;
    view.currentBet = 0;
    view.minRaiseTo = 10;

    EXPECT_EQ(agent.act(view).type, ActionType::Bet);
}

TEST(PolicyAgentTest, Act_LetsAWiderRangeOfHandsPlayWhenAggressive)
{
    auto view = viewWith("Kd 7c", "");
    view.toCall = 40;
    view.currentBet = 40;

    PolicyAgent tight(AgentStyle::Tight, kDefaultHandStrengths,
            kDefaultThresholds);
    PolicyAgent aggressive(AgentStyle::Aggressive, kDefaultHandStrengths,
            kDefaultThresholds);

    EXPECT_EQ(tight.act(view).type, ActionType::Fold);
    EXPECT_NE(aggressive.act(view).type, ActionType::Fold);
}

// A short stack cannot make the minimum raise.
// The only raise left is all-in, and the agent must ask for that.
// Anything else would be rejected by the table.
TEST(PolicyAgentTest, Act_ShovesTheRestWhenItCannotMakeAFullRaise)
{
    PolicyAgent agent(AgentStyle::Balanced, kDefaultHandStrengths,
            kDefaultThresholds);
    auto view = viewWith("Ac Ad", "");
    view.toCall = 40;
    view.currentBet = 40;
    view.minRaiseTo = 200;
    view.maxRaiseTo = 60;
    view.stack = 60;

    const auto action = agent.act(view);
    EXPECT_EQ(action.type, ActionType::Raise);
    EXPECT_EQ(action.amount, 60U);
}

TEST(PolicyAgentTest, Act_CallsInsteadOfRaisingWhenTheStackCannotBeatTheBet)
{
    PolicyAgent agent(AgentStyle::Balanced, kDefaultHandStrengths,
            kDefaultThresholds);
    auto view = viewWith("Ac Ad", "");
    view.toCall = 40;
    view.currentBet = 100;
    view.maxRaiseTo = 100;

    EXPECT_EQ(agent.act(view).type, ActionType::Call);
}

TEST(PolicyAgentTest, Act_CallsInsteadOfRaisingWhenTheBettingIsNotReopened)
{
    PolicyAgent agent(AgentStyle::Balanced, kDefaultHandStrengths,
            kDefaultThresholds);
    auto view = viewWith("Ac Ad", "");
    view.toCall = 40;
    view.currentBet = 40;
    view.mayRaise = false;

    EXPECT_EQ(agent.act(view).type, ActionType::Call);
}

// One property matters more here than any single decision.
// Whatever the table asks, the answer is always legal.
// A few hundred real hands sweep positions, stacks and short all-ins.
// That is broad enough to be worth asserting as one test.
TEST(PolicyAgentTest, Act_NeverReturnsAnActionTheTableWouldReject)
{
    Table table(4, Blinds{.small = 5, .big = 10});
    for (std::size_t index = 0; index < table.seatCount(); ++index)
    {
        table.seatPlayer(makeSeatId(index), 300);
    }

    std::vector<PolicyAgent> agents;
    agents.emplace_back(AgentStyle::Tight, kDefaultHandStrengths,
            kDefaultThresholds);
    agents.emplace_back(AgentStyle::Balanced, kDefaultHandStrengths,
            kDefaultThresholds);
    agents.emplace_back(AgentStyle::Aggressive, kDefaultHandStrengths,
            kDefaultThresholds);
    agents.emplace_back(AgentStyle::Balanced, kDefaultHandStrengths,
            kDefaultThresholds);
    std::vector<std::reference_wrapper<IAgent>> refs;
    for (auto &agent : agents)
    {
        refs.emplace_back(agent);
    }

    SplitMix64Rng rng(20260729);
    Deck deck(rng);
    TableRunner runner(table, deck, std::move(refs));

    std::size_t handsSeen = 0;
    for (std::size_t step = 0; step < 4000; ++step)
    {
        const auto outcome = runner.step();
        if (outcome.kind == StepKind::HandCompleted)
        {
            ++handsSeen;
        }
        if (outcome.kind == StepKind::TableIdle)
        {
            break;
        }
    }

    // Chips only move between the four stacks.
    // So the total is fixed however the poker went.
    antwika::holdem::Chips total = table.pot();
    for (std::size_t index = 0; index < table.seatCount(); ++index)
    {
        total += table.seatAt(makeSeatId(index)).stack;
    }

    EXPECT_GT(handsSeen, 20U);
    EXPECT_EQ(total, 1200U);
}
