#include "antwika/pattern/Cycle.hpp"

#include <cstdint>
#include <limits>

#include <gtest/gtest.h>

#include "antwika/pattern/PatternError.hpp"

using antwika::pattern::Cycle;
using antwika::pattern::PatternError;

namespace
{
    constexpr auto kMost = std::numeric_limits<std::int64_t>::max();
    constexpr auto kLeast = std::numeric_limits<std::int64_t>::min();
} // namespace

TEST(CycleTest, DefaultsToTheVeryStart)
{
    const Cycle start;

    EXPECT_EQ(start.numerator(), 0);
    EXPECT_EQ(start.denominator(), 1);
}

TEST(CycleTest, HoldsAWholeNumberOfCycles)
{
    const Cycle third(3);

    EXPECT_EQ(third.numerator(), 3);
    EXPECT_EQ(third.denominator(), 1);
}

TEST(CycleTest, ReducesOnTheWayIn)
{
    const Cycle half(2, 4);

    EXPECT_EQ(half.numerator(), 1);
    EXPECT_EQ(half.denominator(), 2);
}

TEST(CycleTest, ZeroReducesToZeroOverOne)
{
    const Cycle nothing(0, 7);

    EXPECT_EQ(nothing.numerator(), 0);
    EXPECT_EQ(nothing.denominator(), 1);
}

// The sign lives in the numerator.
// Ordering and reduction each have one case rather than four.
TEST(CycleTest, MovesTheSignOntoTheNumerator)
{
    const Cycle back(1, -2);

    EXPECT_EQ(back.numerator(), -1);
    EXPECT_EQ(back.denominator(), 2);
}

TEST(CycleTest, RefusesADenominatorOfZero)
{
    EXPECT_THROW(Cycle(1, 0), PatternError);
}

TEST(CycleTest, RefusesASignItCannotMove)
{
    EXPECT_THROW(Cycle(kLeast, -1), PatternError);
    EXPECT_THROW(Cycle(1, kLeast), PatternError);
}

// The most negative integer is what std::gcd may not be handed.
// A numerator is allowed to hold it.
TEST(CycleTest, ReducesTheMostNegativeNumeratorWithoutTripping)
{
    const Cycle deep(kLeast, 2);

    EXPECT_EQ(deep.numerator(), kLeast / 2);
    EXPECT_EQ(deep.denominator(), 1);
}

// Floored rather than truncated:
// A position before zero belongs to the cycle it is inside.
TEST(CycleTest, SaysWhichCycleAPositionFallsIn)
{
    EXPECT_EQ(Cycle(1, 2).floorCycle(), 0);
    EXPECT_EQ(Cycle(3, 2).floorCycle(), 1);
    EXPECT_EQ(Cycle(2).floorCycle(), 2);
    EXPECT_EQ(Cycle(-1, 2).floorCycle(), -1);
    EXPECT_EQ(Cycle(-3, 2).floorCycle(), -2);
    EXPECT_EQ(Cycle(-2).floorCycle(), -2);
}

TEST(CycleTest, FindsTheStartOfItsOwnCycleAndTheNext)
{
    EXPECT_EQ(Cycle(3, 2).sam(), Cycle(1));
    EXPECT_EQ(Cycle(3, 2).nextSam(), Cycle(2));
    EXPECT_EQ(Cycle(2).sam(), Cycle(2));
    EXPECT_EQ(Cycle(2).nextSam(), Cycle(3));
}

TEST(CycleTest, RefusesANextCycleItCannotReach)
{
    EXPECT_THROW((void)Cycle(kMost).nextSam(), PatternError);
}

TEST(CycleTest, AddsAndSubtractsExactly)
{
    EXPECT_EQ(Cycle(1, 3) + Cycle(1, 6), Cycle(1, 2));
    EXPECT_EQ(Cycle(1, 2) - Cycle(1, 3), Cycle(1, 6));
    EXPECT_EQ(Cycle(1, 3) - Cycle(1, 2), Cycle(-1, 6));
}

TEST(CycleTest, MultipliesAndDividesExactly)
{
    EXPECT_EQ(Cycle(2, 3) * Cycle(3, 4), Cycle(1, 2));
    EXPECT_EQ(Cycle(1, 2) / Cycle(3, 2), Cycle(1, 3));
    EXPECT_EQ(Cycle(1, 2) / Cycle(-1, 2), Cycle(-1));
}

TEST(CycleTest, RefusesToDivideByNothing)
{
    EXPECT_THROW((void)(Cycle(1, 2) / Cycle()), PatternError);
}

// Composing time transformations grows a denominator.
// One that outgrows its integers is refused rather than rounded.
TEST(CycleTest, RefusesArithmeticThatWillNotFit)
{
    const Cycle small(1, 4000000000);

    EXPECT_THROW((void)(small * small), PatternError);
    EXPECT_THROW((void)(small + Cycle(1, 4000000001)), PatternError);
    EXPECT_THROW((void)(Cycle(kMost) + Cycle(1)), PatternError);
    EXPECT_THROW((void)(Cycle(kLeast) - Cycle(1)), PatternError);
    EXPECT_THROW((void)(Cycle(4000000000) / small), PatternError);
}

// The opposite call from antwika::animation::Progress.
// That type compares on the pair.
// This is a position, so one half and two quarters are one moment.
TEST(CycleTest, ComparesByValueRatherThanByPair)
{
    EXPECT_EQ(Cycle(1, 2), Cycle(2, 4));
    EXPECT_NE(Cycle(1, 2), Cycle(1, 3));
}

TEST(CycleTest, OrdersByValue)
{
    EXPECT_LT(Cycle(1, 3), Cycle(1, 2));
    EXPECT_GT(Cycle(3, 2), Cycle(1));
    EXPECT_LE(Cycle(1, 2), Cycle(2, 4));
    EXPECT_GE(Cycle(1, 2), Cycle(2, 4));
    EXPECT_LT(Cycle(-1), Cycle());
}

// Ordering is total and never throws, however large the denominators.
// It walks rather than widening.
// Cross-multiplying these would need an integer twice as wide.
TEST(CycleTest, OrdersHugeDenominatorsWithoutThrowing)
{
    EXPECT_LT(Cycle(1, kMost), Cycle(1, kMost - 1));
    EXPECT_GT(Cycle(kMost, 2), Cycle(kMost, 3));
}

// The comparison reverses each time it takes a reciprocal.
// A pair resolved on the second round answers the other way about.
TEST(CycleTest, OrdersPairsThatResolveAfterAReciprocal)
{
    EXPECT_LT(Cycle(1), Cycle(3, 2));
    EXPECT_GT(Cycle(3, 2), Cycle(1));

    EXPECT_GT(Cycle(1, 2), Cycle(2, 5));
    EXPECT_LT(Cycle(2, 5), Cycle(1, 2));

    EXPECT_LT(Cycle(1, 3), Cycle(1, 2));
    EXPECT_GT(Cycle(1, 2), Cycle(1, 3));
}
