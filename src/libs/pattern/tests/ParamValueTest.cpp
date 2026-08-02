#include "antwika/pattern/ParamValue.hpp"

#include <cstdint>
#include <limits>

#include <gtest/gtest.h>

#include "antwika/pattern/PatternError.hpp"

using antwika::pattern::kFractionBits;
using antwika::pattern::ParamValue;
using antwika::pattern::PatternError;

namespace
{
    constexpr std::int64_t kOne = std::int64_t{1} << kFractionBits;
    constexpr auto kMost = std::numeric_limits<std::int64_t>::max();
} // namespace

TEST(ParamValueTest, DefaultsToNothing)
{
    EXPECT_EQ(ParamValue{}.raw(), 0);
    EXPECT_DOUBLE_EQ(ParamValue{}.approximate(), 0.0);
}

TEST(ParamValueTest, HoldsAWholeNumber)
{
    EXPECT_EQ(ParamValue(3).raw(), 3 * kOne);
    EXPECT_DOUBLE_EQ(ParamValue(3).approximate(), 3.0);
}

// Scaled before dividing, so a fraction keeps what lies below the point.
TEST(ParamValueTest, HoldsAFraction)
{
    EXPECT_EQ(ParamValue(1, 2).raw(), kOne / 2);
    EXPECT_DOUBLE_EQ(ParamValue(1, 4).approximate(), 0.25);
    EXPECT_DOUBLE_EQ(ParamValue(-3, 4).approximate(), -0.75);
}

TEST(ParamValueTest, RefusesADenominatorOfZero)
{
    EXPECT_THROW(ParamValue(1, 0), PatternError);
}

// The sign lives on the numerator, so a negative denominator reads
// the same as its mirrored fraction.
TEST(ParamValueTest, MovesTheSignToTheNumerator)
{
    EXPECT_EQ(ParamValue(1, -2), ParamValue(-1, 2));
    EXPECT_EQ(ParamValue(-1, -2), ParamValue(1, 2));
}

// The scaled minimum over minus one is the one division the hardware
// traps on rather than throws; every route to it must refuse first.
TEST(ParamValueTest, RefusesAFractionWhoseSignCannotMove)
{
    constexpr auto kLeast = std::numeric_limits<std::int64_t>::min();

    EXPECT_THROW((ParamValue{kLeast, -1}), PatternError);
    EXPECT_THROW((ParamValue{1, kLeast}), PatternError);
    EXPECT_THROW((ParamValue{-2147483648LL, -1}), PatternError);
}

TEST(ParamValueTest, RefusesAWholeNumberThatWillNotFit)
{
    EXPECT_THROW(ParamValue{kMost}, PatternError);
    EXPECT_THROW((ParamValue{kMost, 2}), PatternError);
}

TEST(ParamValueTest, RoundTripsThroughItsBits)
{
    const auto half = ParamValue(1, 2);

    EXPECT_EQ(ParamValue::fromRaw(half.raw()), half);
}

TEST(ParamValueTest, AddsAndSubtracts)
{
    EXPECT_EQ(ParamValue(1, 4) + ParamValue(1, 4), ParamValue(1, 2));
    EXPECT_EQ(ParamValue(1) - ParamValue(1, 4), ParamValue(3, 4));
}

TEST(ParamValueTest, RefusesArithmeticThatWillNotFit)
{
    const auto huge = ParamValue::fromRaw(kMost);

    EXPECT_THROW((void)(huge + huge), PatternError);
    EXPECT_THROW(
        (void)(ParamValue::fromRaw(
                   std::numeric_limits<std::int64_t>::min())
               - huge),
        PatternError);
}

TEST(ParamValueTest, ComparesAndOrders)
{
    EXPECT_EQ(ParamValue(1, 2), ParamValue(2, 4));
    EXPECT_NE(ParamValue(1, 2), ParamValue(1, 3));
    EXPECT_LT(ParamValue(1, 3), ParamValue(1, 2));
    EXPECT_GT(ParamValue(1), ParamValue(1, 2));
}

TEST(ParamValueTest, OrdersEqualValuesAsNeitherLessNorGreater)
{
    EXPECT_LE(ParamValue(1, 2), ParamValue(2, 4));
    EXPECT_GE(ParamValue(1, 2), ParamValue(2, 4));
    EXPECT_FALSE(ParamValue(1, 2) < ParamValue(2, 4));
    EXPECT_FALSE(ParamValue(1, 2) > ParamValue(2, 4));
}
