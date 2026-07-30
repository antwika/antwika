#include <gtest/gtest.h>

#include <antwika/holdem/CardText.hpp>
#include <antwika/holdem/HandFlow.hpp>
#include <antwika/holdem/Stage.hpp>
#include <antwika/holdem/TableStateError.hpp>
#include <antwika/holdem/fakes/FakeDeck.hpp>

using antwika::holdem::HandFlow;
using antwika::holdem::parseCards;
using antwika::holdem::Stage;
using antwika::holdem::TableStateError;
using antwika::holdem::fakes::FakeDeck;

namespace
{
    // Two hole cards, then a five-card board, in dealing order.
    [[nodiscard]] FakeDeck scriptedDeck()
    {
        return FakeDeck(parseCards("Ac Kc 2d 7h Ts 3c 9s"));
    }
} // namespace

TEST(HandFlowTest, Stage_StartsPreFlopBeforeAnyHand)
{
    const HandFlow flow;

    EXPECT_EQ(flow.stage(), Stage::PreFlop);
    EXPECT_TRUE(flow.board().empty());
}

TEST(HandFlowTest, Begin_ShufflesTheDeckItIsHanded)
{
    auto deck = scriptedDeck();
    HandFlow flow;

    flow.begin(deck);

    EXPECT_EQ(deck.shuffleCount(), 1U);
}

TEST(HandFlowTest, Begin_ClearsTheBoardTheLastHandLeft)
{
    auto deck = scriptedDeck();
    HandFlow flow;
    flow.begin(deck);
    flow.dealStreet();
    ASSERT_EQ(flow.board().size(), 3U);

    auto second = scriptedDeck();
    flow.begin(second);

    EXPECT_TRUE(flow.board().empty());
    EXPECT_EQ(flow.stage(), Stage::PreFlop);
}

TEST(HandFlowTest, DealCard_TakesTheNextCardOffTheDeck)
{
    auto deck = scriptedDeck();
    HandFlow flow;
    flow.begin(deck);

    EXPECT_EQ(flow.dealCard(), parseCards("Ac").front());
    EXPECT_EQ(flow.dealCard(), parseCards("Kc").front());

    // A card dealt to a player is not on the board.
    EXPECT_TRUE(flow.board().empty());
}

TEST(HandFlowTest, DealCard_ThrowsBeforeAnyHandHasBegun)
{
    HandFlow flow;

    EXPECT_THROW((void)flow.dealCard(), TableStateError);
}

TEST(HandFlowTest, DealStreet_TurnsThreeCardsOnTheFlop)
{
    auto deck = scriptedDeck();
    HandFlow flow;
    flow.begin(deck);

    flow.dealStreet();

    EXPECT_EQ(flow.stage(), Stage::Flop);
    EXPECT_EQ(flow.board(), parseCards("Ac Kc 2d"));
}

TEST(HandFlowTest, DealStreet_TurnsOneCardOnEveryStreetAfterTheFlop)
{
    auto deck = scriptedDeck();
    HandFlow flow;
    flow.begin(deck);
    flow.dealStreet();

    flow.dealStreet();
    EXPECT_EQ(flow.stage(), Stage::Turn);
    EXPECT_EQ(flow.board().size(), 4U);

    flow.dealStreet();
    EXPECT_EQ(flow.stage(), Stage::River);
    EXPECT_EQ(flow.board(), parseCards("Ac Kc 2d 7h Ts"));
}

TEST(HandFlowTest, DealStreet_ThrowsOnceTheWholeBoardIsOut)
{
    auto deck = scriptedDeck();
    HandFlow flow;
    flow.begin(deck);
    flow.dealStreet();
    flow.dealStreet();
    flow.dealStreet();
    ASSERT_FALSE(flow.hasStreetToDeal());

    EXPECT_THROW(flow.dealStreet(), TableStateError);
}

TEST(HandFlowTest, DealStreet_ThrowsBeforeAnyHandHasBegun)
{
    HandFlow flow;

    EXPECT_THROW(flow.dealStreet(), TableStateError);

    // The deck is asked for first, so the stage did not move.
    EXPECT_EQ(flow.stage(), Stage::PreFlop);
}

TEST(HandFlowTest, HasStreetToDeal_IsTrueUntilTheRiverIsOut)
{
    auto deck = scriptedDeck();
    HandFlow flow;
    flow.begin(deck);

    EXPECT_TRUE(flow.hasStreetToDeal());
    flow.dealStreet();
    EXPECT_TRUE(flow.hasStreetToDeal());
}

TEST(HandFlowTest, ToShowdown_ReachesTheShowdownWithoutDealing)
{
    auto deck = scriptedDeck();
    HandFlow flow;
    flow.begin(deck);
    flow.dealStreet();

    flow.toShowdown();

    EXPECT_EQ(flow.stage(), Stage::Showdown);
    EXPECT_EQ(flow.board().size(), 3U);

    // A hand that got here has nothing left to deal, board or not.
    EXPECT_FALSE(flow.hasStreetToDeal());
}

TEST(HandFlowTest, End_LetsGoOfTheDeck)
{
    auto deck = scriptedDeck();
    HandFlow flow;
    flow.begin(deck);
    flow.dealStreet();

    flow.end();

    EXPECT_THROW((void)flow.dealCard(), TableStateError);

    // What the hand came to survives it, so it can still be reported.
    EXPECT_EQ(flow.stage(), Stage::Flop);
    EXPECT_EQ(flow.board().size(), 3U);
}
