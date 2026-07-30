#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

#include <antwika/holdem/CardText.hpp>
#include <antwika/holdem/HandCategory.hpp>
#include <antwika/holdem/HandEvaluator.hpp>
#include <antwika/holdem/HandText.hpp>
#include <antwika/holdem/HandValue.hpp>

using antwika::holdem::describe;
using antwika::holdem::evaluate;
using antwika::holdem::HandCategory;
using antwika::holdem::HandValue;
using antwika::holdem::kCategoryShift;
using antwika::holdem::kSlotBits;
using antwika::holdem::parseCards;

namespace
{
    [[nodiscard]] std::string describeCards(std::string_view cards)
    {
        return describe(evaluate(parseCards(cards)));
    }

    [[nodiscard]] HandValue pairOf(std::uint32_t rank)
    {
        const auto slot = kCategoryShift - kSlotBits;
        return static_cast<HandValue>(
            (static_cast<std::uint32_t>(HandCategory::OnePair)
             << kCategoryShift)
            | (rank << slot));
    }
} // namespace

TEST(HandTextTest, Describe_NamesTheHighCardOfAHandWithNothingElse)
{
    EXPECT_EQ(describeCards("Ah Kd 9c 7s 3h"), "high card Ace");
}

TEST(HandTextTest, Describe_NamesThePairedRank)
{
    EXPECT_EQ(describeCards("7h 7d Ac Kd 3s"), "a pair of Sevens");
}

TEST(HandTextTest, Describe_NamesBothPairsHighestFirst)
{
    EXPECT_EQ(
        describeCards("Kh Kd 4c 4d 9s"), "two pair, Kings and Fours");
}

TEST(HandTextTest, Describe_NamesTheTrippedRank)
{
    EXPECT_EQ(describeCards("Qh Qd Qc 9s 3h"), "three of a kind, Queens");
}

TEST(HandTextTest, Describe_NamesAStraightByItsEnds)
{
    EXPECT_EQ(describeCards("5h 6d 7c 8s 9h"), "a straight, Five to Nine");
}

// The wheel is the one straight whose ace plays low.
// So it reads from the ace even though the five is its top card.
TEST(HandTextTest, Describe_ReadsTheWheelFromItsAce)
{
    EXPECT_EQ(describeCards("Ah 2d 3c 4s 5h"), "a straight, Ace to Five");
}

TEST(HandTextTest, Describe_NamesAFlushByItsHighestCard)
{
    EXPECT_EQ(describeCards("Ah 9h 7h 4h 2h"), "a flush, Ace high");
}

TEST(HandTextTest, Describe_NamesBothHalvesOfAFullHouse)
{
    EXPECT_EQ(
        describeCards("Th Td Tc 6s 6h"), "a full house, Tens full of Sixes");
}

TEST(HandTextTest, Describe_NamesTheQuadedRank)
{
    EXPECT_EQ(describeCards("9h 9d 9c 9s 3h"), "four of a kind, Nines");
}

TEST(HandTextTest, Describe_NamesAStraightFlushByItsEnds)
{
    EXPECT_EQ(
        describeCards("9h Th Jh Qh Kh"), "a straight flush, Nine to King");
}

TEST(HandTextTest, Describe_NamesEveryRank)
{
    constexpr std::array<std::string_view, 13> names{
        "Deuces",
        "Threes",
        "Fours",
        "Fives",
        "Sixes",
        "Sevens",
        "Eights",
        "Nines",
        "Tens",
        "Jacks",
        "Queens",
        "Kings",
        "Aces",
    };

    for (std::uint32_t rank = 0; rank < names.size(); ++rank)
    {
        EXPECT_EQ(
            describe(pairOf(rank)),
            "a pair of " + std::string(names[rank]));
    }
}

TEST(HandTextTest, Describe_FallsBackForAValueNamingNoRank)
{
    EXPECT_EQ(describe(pairOf(13)), "a pair of Unknowns");
}

TEST(HandTextTest, Describe_FallsBackForAValueNamingNoCategory)
{
    const auto value = static_cast<HandValue>(
        std::uint32_t{42} << kCategoryShift);
    EXPECT_EQ(describe(value), "an unknown hand");
}
