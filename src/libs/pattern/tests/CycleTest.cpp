#include <gtest/gtest.h>

#include <cstdint>
#include <limits>

#include "antwika/pattern/Cycle.hpp"
#include "antwika/pattern/PatternError.hpp"

using antwika::pattern::Cycle;
using antwika::pattern::PatternError;

namespace
{
    constexpr auto kMost = std::numeric_limits<std::int64_t>::max();
    constexpr auto kLeast = std::numeric_limits<std::int64_t>::min();
}

TEST(CycleTest, Ctor_DefaultsToTheVeryStart)
{
    const Cycle start;

    EXPECT_EQ(start.numerator(), 0);
    EXPECT_EQ(start.denominator(), 1);
}

TEST(CycleTest, Ctor_HoldsAWholeNumberOfCycles)
{
    const Cycle third(3);

    EXPECT_EQ(third.numerator(), 3);
    EXPECT_EQ(third.denominator(), 1);
}

TEST(CycleTest, Ctor_ReducesOnTheWayIn)
{
    const Cycle half(2, 4);

    EXPECT_EQ(half.numerator(), 1);
    EXPECT_EQ(half.denominator(), 2);
}

TEST(CycleTest, Ctor_ReducesZeroToZeroOverOne)
{
    const Cycle nothing(0, 7);

    EXPECT_EQ(nothing.numerator(), 0);
    EXPECT_EQ(nothing.denominator(), 1);
}

TEST(CycleTest, Ctor_MovesTheSignOntoTheNumerator)
{
    const Cycle back(1, -2);

    EXPECT_EQ(back.numerator(), -1);
    EXPECT_EQ(back.denominator(), 2);
}

TEST(CycleTest, Ctor_RefusesADenominatorOfZero)
{
    EXPECT_THROW(Cycle(1, 0), PatternError);
}

TEST(CycleTest, Ctor_RefusesASignItCannotMove)
{
    EXPECT_THROW(Cycle(kLeast, -1), PatternError);
    EXPECT_THROW(Cycle(1, kLeast), PatternError);
}

TEST(CycleTest, Ctor_ReducesTheMostNegative)
{
    const Cycle deep(kLeast, 2);

    EXPECT_EQ(deep.numerator(), kLeast / 2);
    EXPECT_EQ(deep.denominator(), 1);
}

TEST(CycleTest, FloorCycle_SaysWhichCycleAPositionIsIn)
{
    EXPECT_EQ(Cycle(1, 2).floorCycle(), 0);
    EXPECT_EQ(Cycle(3, 2).floorCycle(), 1);
    EXPECT_EQ(Cycle(2).floorCycle(), 2);
    EXPECT_EQ(Cycle(-1, 2).floorCycle(), -1);
    EXPECT_EQ(Cycle(-3, 2).floorCycle(), -2);
    EXPECT_EQ(Cycle(-2).floorCycle(), -2);
}

TEST(CycleTest, Sam_FindsTheStartOfItsOwnCycle)
{
    EXPECT_EQ(Cycle(3, 2).sam(), Cycle(1));
    EXPECT_EQ(Cycle(2).sam(), Cycle(2));
    EXPECT_EQ(Cycle(-1, 2).sam(), Cycle(-1));
}

TEST(CycleTest, NextSam_FindsTheStartOfTheCycleAfter)
{
    EXPECT_EQ(Cycle(3, 2).nextSam(), Cycle(2));
    EXPECT_EQ(Cycle(2).nextSam(), Cycle(3));
    EXPECT_EQ(Cycle(-1, 2).nextSam(), Cycle());
}

TEST(CycleTest, NextSam_RefusesACycleItCannotReach)
{
    EXPECT_THROW((void)Cycle(kMost).nextSam(), PatternError);
}

TEST(CycleTest, OperatorPlus_AddsExactly)
{
    EXPECT_EQ(Cycle(1, 3) + Cycle(1, 6), Cycle(1, 2));
}

TEST(CycleTest, OperatorMinus_SubtractsExactlyAndPastZero)
{
    EXPECT_EQ(Cycle(1, 2) - Cycle(1, 3), Cycle(1, 6));
    EXPECT_EQ(Cycle(1, 3) - Cycle(1, 2), Cycle(-1, 6));
}

TEST(CycleTest, OperatorTimes_MultipliesExactly)
{
    EXPECT_EQ(Cycle(2, 3) * Cycle(3, 4), Cycle(1, 2));
}

TEST(CycleTest, OperatorDivide_DividesExactlyAndKeepsTheSign)
{
    EXPECT_EQ(Cycle(1, 2) / Cycle(3, 2), Cycle(1, 3));
    EXPECT_EQ(Cycle(1, 2) / Cycle(-1, 2), Cycle(-1));
}

TEST(CycleTest, OperatorDivide_RefusesToDivideByNothing)
{
    EXPECT_THROW((void)(Cycle(1, 2) / Cycle()), PatternError);
}

TEST(CycleTest, OperatorTimes_RefusesWhatWillNotFit)
{
    const Cycle small(1, 4000000000);

    EXPECT_THROW((void)(small * small), PatternError);
}

TEST(CycleTest, OperatorPlus_RefusesWhatWillNotFit)
{
    const Cycle small(1, 4000000000);

    EXPECT_THROW((void)(small + Cycle(1, 4000000001)), PatternError);
    EXPECT_THROW((void)(Cycle(kMost) + Cycle(1)), PatternError);
}

TEST(CycleTest, OperatorMinus_RefusesWhatWillNotFit)
{
    EXPECT_THROW((void)(Cycle(kLeast) - Cycle(1)), PatternError);
}

TEST(CycleTest, OperatorDivide_RefusesWhatWillNotFit)
{
    const Cycle small(1, 4000000000);

    EXPECT_THROW((void)(Cycle(4000000000) / small), PatternError);
}

TEST(CycleTest, OperatorEquals_ComparesByValueNotByPair)
{
    EXPECT_EQ(Cycle(1, 2), Cycle(2, 4));
    EXPECT_NE(Cycle(1, 2), Cycle(1, 3));
}

TEST(CycleTest, OperatorCompare_OrdersByValue)
{
    EXPECT_LT(Cycle(1, 3), Cycle(1, 2));
    EXPECT_GT(Cycle(3, 2), Cycle(1));
    EXPECT_LE(Cycle(1, 2), Cycle(2, 4));
    EXPECT_GE(Cycle(1, 2), Cycle(2, 4));
    EXPECT_LT(Cycle(-1), Cycle());
}

TEST(CycleTest, OperatorCompare_HandlesHugeDenominators)
{
    EXPECT_LT(Cycle(1, kMost), Cycle(1, kMost - 1));
    EXPECT_GT(Cycle(kMost, 2), Cycle(kMost, 3));
}

TEST(CycleTest, OperatorCompare_ResolvesAfterAReciprocal)
{
    EXPECT_LT(Cycle(1), Cycle(3, 2));
    EXPECT_GT(Cycle(3, 2), Cycle(1));

    EXPECT_GT(Cycle(1, 2), Cycle(2, 5));
    EXPECT_LT(Cycle(2, 5), Cycle(1, 2));

    EXPECT_LT(Cycle(1, 3), Cycle(1, 2));
    EXPECT_GT(Cycle(1, 2), Cycle(1, 3));
}
