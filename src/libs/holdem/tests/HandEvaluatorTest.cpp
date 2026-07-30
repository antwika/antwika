#include <gtest/gtest.h>

#include <string_view>
#include <vector>

#include <antwika/holdem/Card.hpp>
#include <antwika/holdem/CardText.hpp>
#include <antwika/holdem/HandCategory.hpp>
#include <antwika/holdem/HandEvaluationError.hpp>
#include <antwika/holdem/HandEvaluator.hpp>
#include <antwika/holdem/HandValue.hpp>

using antwika::holdem::Card;
using antwika::holdem::categoryOf;
using antwika::holdem::evaluate;
using antwika::holdem::HandCategory;
using antwika::holdem::HandEvaluationError;
using antwika::holdem::HandValue;
using antwika::holdem::parseCards;

namespace
{
    [[nodiscard]] HandValue score(std::string_view cards)
    {
        return evaluate(parseCards(cards));
    }

    [[nodiscard]] HandCategory categoryOfHand(std::string_view cards)
    {
        return categoryOf(score(cards));
    }
} // namespace

TEST(HandEvaluatorTest, Evaluate_NamesEveryCategoryFromFiveCards)
{
    EXPECT_EQ(
        categoryOfHand("As Ks Qs Js Ts"), HandCategory::StraightFlush);
    EXPECT_EQ(categoryOfHand("9c 9d 9h 9s 2c"), HandCategory::FourOfAKind);
    EXPECT_EQ(categoryOfHand("9c 9d 9h 4s 4c"), HandCategory::FullHouse);
    EXPECT_EQ(categoryOfHand("Ah 9h 7h 4h 2h"), HandCategory::Flush);
    EXPECT_EQ(categoryOfHand("9c 8d 7h 6s 5c"), HandCategory::Straight);
    EXPECT_EQ(
        categoryOfHand("9c 9d 9h 6s 5c"), HandCategory::ThreeOfAKind);
    EXPECT_EQ(categoryOfHand("9c 9d 6h 6s 5c"), HandCategory::TwoPair);
    EXPECT_EQ(categoryOfHand("9c 9d 7h 6s 5c"), HandCategory::OnePair);
    EXPECT_EQ(categoryOfHand("Ac Jd 9h 6s 5c"), HandCategory::HighCard);
}

// This is the whole point of collapsing a hand to one number.
// Sorting these values sorts the hands, with no other logic.
TEST(HandEvaluatorTest, Evaluate_RanksTheCategoriesInTheRightOrder)
{
    const std::vector<HandValue> ascending{
        score("Ac Jd 9h 6s 5c"),
        score("9c 9d 7h 6s 5c"),
        score("9c 9d 6h 6s 5c"),
        score("9c 9d 9h 6s 5c"),
        score("9c 8d 7h 6s 5c"),
        score("Ah 9h 7h 4h 2h"),
        score("9c 9d 9h 4s 4c"),
        score("9c 9d 9h 9s 2c"),
        score("As Ks Qs Js Ts"),
    };

    for (std::size_t index = 1; index < ascending.size(); ++index)
    {
        EXPECT_LT(ascending[index - 1], ascending[index]) << index;
    }
}

TEST(HandEvaluatorTest, Evaluate_TreatsTheAceAsLowForTheWheel)
{
    EXPECT_EQ(categoryOfHand("5c 4d 3h 2s Ac"), HandCategory::Straight);
    EXPECT_LT(score("5c 4d 3h 2s Ac"), score("6c 5d 4h 3s 2c"));
}

TEST(HandEvaluatorTest, Evaluate_RanksTheWheelBelowEveryOtherStraight)
{
    EXPECT_LT(score("5c 4d 3h 2s Ac"), score("Ac Kd Qh Js Tc"));
}

TEST(HandEvaluatorTest, Evaluate_FindsAStraightFlushOnTheWheel)
{
    EXPECT_EQ(
        categoryOfHand("5c 4c 3c 2c Ac"), HandCategory::StraightFlush);
    EXPECT_LT(score("5c 4c 3c 2c Ac"), score("6c 5c 4c 3c 2c"));
}

TEST(HandEvaluatorTest, Evaluate_ScoresTheSameRanksAsEqualAcrossSuits)
{
    EXPECT_EQ(score("Ac Kd 9h 6s 5c"), score("Ad Kh 9s 6c 5d"));
}

TEST(HandEvaluatorTest, Evaluate_ComparesKickersWhenPairsMatch)
{
    EXPECT_LT(score("Ac Ad Kh Qs Tc"), score("Ac Ad Kh Qs Jc"));
    EXPECT_EQ(score("Ac Ad Kh Qs Jc"), score("Ah As Kc Qd Jh"));
}

TEST(HandEvaluatorTest, Evaluate_ComparesTheKickerOnFourOfAKind)
{
    EXPECT_LT(score("9c 9d 9h 9s 2c"), score("9c 9d 9h 9s 3c"));
}

TEST(HandEvaluatorTest, Evaluate_ComparesTripsBeforeThePairInAFullHouse)
{
    EXPECT_LT(score("8c 8d 8h Ks Kc"), score("9c 9d 9h 2s 2c"));
}

TEST(HandEvaluatorTest, Evaluate_ComparesTheHighPairFirstOnTwoPair)
{
    EXPECT_LT(score("9c 9d 8h 8s Ac"), score("Tc Td 2h 2s 3c"));
}

TEST(HandEvaluatorTest, Evaluate_PicksTheBestFiveOfSevenCards)
{
    // Two pair is available, but so is a flush, and the flush wins.
    EXPECT_EQ(
        categoryOfHand("Ah Kh 9h 4h 2h 9c 4c"), HandCategory::Flush);
    EXPECT_EQ(score("Ah Kh 9h 4h 2h 9c 4c"), score("Ah Kh 9h 4h 2h"));
}

TEST(HandEvaluatorTest, Evaluate_PicksTheHigherOfTwoTripsForAFullHouse)
{
    // Nines full of fours, not fours full of nines.
    EXPECT_EQ(
        score("9c 9d 9h 4s 4c 4d 2s"), score("9c 9d 9h 4s 4c"));
}

TEST(HandEvaluatorTest, Evaluate_TakesTheKickerFromAThirdPair)
{
    // Aces and kings with a queen kicker, from three pairs plus a two.
    EXPECT_EQ(
        score("Ac Ad Kh Ks Qc Qd 2s"), score("Ac Ad Kh Ks Qc"));
}

TEST(HandEvaluatorTest, Evaluate_PrefersASixCardHandsBestFive)
{
    EXPECT_EQ(score("Ac Kd Qh Js Tc 2d"), score("Ac Kd Qh Js Tc"));
}

TEST(HandEvaluatorTest, Evaluate_RanksAFlushAboveAStraightItAlsoHolds)
{
    // A straight in mixed suits alongside five hearts.
    EXPECT_EQ(
        categoryOfHand("9h 8h 7h 2h 3h 6c 5d"), HandCategory::Flush);
}

TEST(HandEvaluatorTest, Evaluate_RejectsTooFewCards)
{
    EXPECT_THROW(
        static_cast<void>(score("Ac Kd Qh Js")), HandEvaluationError);
}

TEST(HandEvaluatorTest, Evaluate_RejectsTooManyCards)
{
    EXPECT_THROW(
        static_cast<void>(score("Ac Kd Qh Js Tc 9d 8h 7s")),
        HandEvaluationError);
}

TEST(HandEvaluatorTest, Evaluate_RejectsTheSameCardTwice)
{
    EXPECT_THROW(
        static_cast<void>(score("Ac Ac Kd Qh Js")),
        HandEvaluationError);
}

TEST(HandEvaluatorTest, Evaluate_RejectsACardOutsideTheDeck)
{
    const std::vector<Card> cards{
        static_cast<Card>(52),
        static_cast<Card>(0),
        static_cast<Card>(1),
        static_cast<Card>(2),
        static_cast<Card>(3),
    };
    EXPECT_THROW(static_cast<void>(evaluate(cards)), HandEvaluationError);
}
