#include <gtest/gtest.h>

#include <cstdint>
#include <limits>

#include "antwika/pattern/ParamValue.hpp"
#include "antwika/pattern/PatternError.hpp"

using antwika::pattern::kFractionBits;
using antwika::pattern::ParamValue;
using antwika::pattern::PatternError;

namespace
{
    constexpr std::int64_t kOne = std::int64_t{1} << kFractionBits;
    constexpr auto kMost = std::numeric_limits<std::int64_t>::max();
}

TEST(ParamValueTest, Ctor_DefaultsToNothing)
{
    EXPECT_EQ(ParamValue{}.raw(), 0);
    EXPECT_DOUBLE_EQ(ParamValue{}.approximate(), 0.0);
}

TEST(ParamValueTest, Ctor_HoldsAWholeNumber)
{
    EXPECT_EQ(ParamValue(3).raw(), 3 * kOne);
    EXPECT_DOUBLE_EQ(ParamValue(3).approximate(), 3.0);
}

TEST(ParamValueTest, Ctor_HoldsAFraction)
{
    EXPECT_EQ(ParamValue(1, 2).raw(), kOne / 2);
    EXPECT_DOUBLE_EQ(ParamValue(1, 4).approximate(), 0.25);
    EXPECT_DOUBLE_EQ(ParamValue(-3, 4).approximate(), -0.75);
}

TEST(ParamValueTest, Ctor_RefusesADenominatorOfZero)
{
    EXPECT_THROW(ParamValue(1, 0), PatternError);
}

TEST(ParamValueTest, Ctor_MovesTheSignToTheNumerator)
{
    EXPECT_EQ(ParamValue(1, -2), ParamValue(-1, 2));
    EXPECT_EQ(ParamValue(-1, -2), ParamValue(1, 2));
}

TEST(ParamValueTest, Ctor_RefusesASignItCannotMove)
{
    constexpr auto kLeast = std::numeric_limits<std::int64_t>::min();

    EXPECT_THROW((ParamValue{kLeast, -1}), PatternError);
    EXPECT_THROW((ParamValue{1, kLeast}), PatternError);
    EXPECT_THROW((ParamValue{-2147483648LL, -1}), PatternError);
}

TEST(ParamValueTest, Ctor_RefusesAWholeThatWillNotFit)
{
    EXPECT_THROW(ParamValue{kMost}, PatternError);
    EXPECT_THROW((ParamValue{kMost, 2}), PatternError);
}

TEST(ParamValueTest, FromRaw_RoundTripsThroughItsBits)
{
    const auto half = ParamValue(1, 2);

    EXPECT_EQ(ParamValue::fromRaw(half.raw()), half);
}

TEST(ParamValueTest, OperatorPlus_Adds)
{
    EXPECT_EQ(ParamValue(1, 4) + ParamValue(1, 4), ParamValue(1, 2));
}

TEST(ParamValueTest, OperatorMinus_Subtracts)
{
    EXPECT_EQ(ParamValue(1) - ParamValue(1, 4), ParamValue(3, 4));
}

TEST(ParamValueTest, OperatorPlus_RefusesWhatWillNotFit)
{
    const auto huge = ParamValue::fromRaw(kMost);

    EXPECT_THROW((void)(huge + huge), PatternError);
}

TEST(ParamValueTest, OperatorMinus_RefusesWhatWillNotFit)
{
    const auto huge = ParamValue::fromRaw(kMost);

    EXPECT_THROW(
        (void)(ParamValue::fromRaw(
                   std::numeric_limits<std::int64_t>::min())
               - huge),
        PatternError);
}

TEST(ParamValueTest, OperatorEquals_ComparesByValueNotByBits)
{
    EXPECT_EQ(ParamValue(1, 2), ParamValue(2, 4));
    EXPECT_NE(ParamValue(1, 2), ParamValue(1, 3));
}

TEST(ParamValueTest, OperatorCompare_OrdersByValue)
{
    EXPECT_LT(ParamValue(1, 3), ParamValue(1, 2));
    EXPECT_GT(ParamValue(1), ParamValue(1, 2));
}

TEST(ParamValueTest, OperatorCompare_MakesEqualValuesNeither)
{
    EXPECT_LE(ParamValue(1, 2), ParamValue(2, 4));
    EXPECT_GE(ParamValue(1, 2), ParamValue(2, 4));
    EXPECT_FALSE(ParamValue(1, 2) < ParamValue(2, 4));
    EXPECT_FALSE(ParamValue(1, 2) > ParamValue(2, 4));
}
