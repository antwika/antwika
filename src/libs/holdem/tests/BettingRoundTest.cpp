#include <gtest/gtest.h>

#include <antwika/holdem/BettingRound.hpp>
#include <antwika/holdem/IllegalActionError.hpp>

using antwika::holdem::BettingRound;
using antwika::holdem::IllegalActionError;

namespace
{
    constexpr antwika::holdem::Chips kBigBlind = 10;

    [[nodiscard]] BettingRound preFlop()
    {
        BettingRound round;
        round.open(kBigBlind);

        return round;
    }
} // namespace

TEST(BettingRoundTest, Open_MakesTheBigBlindTheLiveBet)
{
    const auto round = preFlop();

    EXPECT_EQ(round.bet(), kBigBlind);
    EXPECT_TRUE(round.isLive());
}

TEST(BettingRoundTest, Open_MakesTheBigBlindTheSmallestRaise)
{
    // A pre-flop raise has to double the blind, not merely beat it.
    EXPECT_EQ(preFlop().minimumRaiseTo(), 2 * kBigBlind);
}

TEST(BettingRoundTest, Reset_LeavesNothingLiveOnANewStreet)
{
    auto round = preFlop();

    round.reset(kBigBlind);

    EXPECT_EQ(round.bet(), 0);
    EXPECT_FALSE(round.isLive());
}

TEST(BettingRoundTest, Reset_KeepsTheBigBlindAsTheSmallestBet)
{
    auto round = preFlop();
    ASSERT_TRUE(round.raiseTo(40, 400));

    round.reset(kBigBlind);

    // Not 30, which is what the last street's raise size would give.
    EXPECT_EQ(round.minimumRaiseTo(), kBigBlind);
}

TEST(BettingRoundTest, Close_ClearsTheLiveBet)
{
    auto round = preFlop();

    round.close();

    EXPECT_FALSE(round.isLive());
}

TEST(BettingRoundTest, OwedBy_IsTheDifferenceFromTheLiveBet)
{
    EXPECT_EQ(preFlop().owedBy(4), kBigBlind - 4);
    EXPECT_EQ(preFlop().owedBy(kBigBlind), 0);
}

TEST(BettingRoundTest, IsCovered_HoldsExactlyWhenNothingIsOwed)
{
    EXPECT_FALSE(preFlop().isCovered(4));
    EXPECT_TRUE(preFlop().isCovered(kBigBlind));
    EXPECT_TRUE(preFlop().isCovered(kBigBlind + 1));
}

TEST(BettingRoundTest, RaiseTo_AFullRaiseReopensTheBetting)
{
    auto round = preFlop();

    EXPECT_TRUE(round.raiseTo(30, 400));
    EXPECT_EQ(round.bet(), 30);
    // The raise was 20, so the next one has to add 20 again.
    EXPECT_EQ(round.minimumRaiseTo(), 50);
}

TEST(BettingRoundTest, RaiseTo_AnAllInShortOfAFullRaiseReopensNothing)
{
    auto round = preFlop();

    // 15 is short of the 20 a full raise needs, and is the whole stack.
    EXPECT_FALSE(round.raiseTo(15, 15));
    EXPECT_EQ(round.bet(), 15);
    // So the minimum raise is still measured from the last full one.
    EXPECT_EQ(round.minimumRaiseTo(), 25);
}

TEST(BettingRoundTest, RaiseTo_RefusesATargetThatDoesNotBeatTheLiveBet)
{
    auto round = preFlop();

    EXPECT_THROW(
        static_cast<void>(round.raiseTo(kBigBlind, 400)),
        IllegalActionError);
}

TEST(BettingRoundTest, RaiseTo_RefusesToUndercutTheMinimumWithChipsInHand)
{
    auto round = preFlop();

    // 15 is short of 20, and the stack reaches 400, so it is a choice.
    EXPECT_THROW(
        static_cast<void>(round.raiseTo(15, 400)), IllegalActionError);
}

TEST(BettingRoundTest, RaiseTo_LeavesTheRoundAloneWhenItRefuses)
{
    auto round = preFlop();

    EXPECT_THROW(
        static_cast<void>(round.raiseTo(15, 400)), IllegalActionError);

    EXPECT_EQ(round.bet(), kBigBlind);
    EXPECT_EQ(round.minimumRaiseTo(), 2 * kBigBlind);
}
