#include <gtest/gtest.h>

#include <cstdint>

#include <antwika/animation/AnimationError.hpp>
#include <antwika/animation/Progress.hpp>

namespace antwika::animation
{

    TEST(ProgressTest, Construct_DefaultsToZeroOverOne)
    {
        const Progress progress;

        EXPECT_EQ(progress.numerator(), 0U);
        EXPECT_EQ(progress.denominator(), 1U);
    }

    TEST(ProgressTest, Construct_KeepsTheFractionItWasGiven)
    {
        const Progress progress(3, 4);

        EXPECT_EQ(progress.numerator(), 3U);
        EXPECT_EQ(progress.denominator(), 4U);
    }

    TEST(ProgressTest, Construct_AcceptsAFullFraction)
    {
        const Progress progress(4, 4);

        EXPECT_EQ(progress.numerator(), 4U);
        EXPECT_EQ(progress.denominator(), 4U);
    }

    TEST(ProgressTest, Construct_ThrowsOnAZeroDenominator)
    {
        EXPECT_THROW(Progress(0, 0), AnimationError);
    }

    TEST(ProgressTest, Construct_ThrowsWhenTheNumeratorExceedsTheWhole)
    {
        EXPECT_THROW(Progress(5, 4), AnimationError);
    }

    TEST(ProgressTest, Equality_ComparesBothFields)
    {
        EXPECT_EQ(Progress(1, 2), Progress(1, 2));
        EXPECT_NE(Progress(1, 2), Progress(2, 4));
        EXPECT_NE(Progress(1, 2), Progress(0, 2));
        EXPECT_NE(Progress(1, 2), Progress(1, 4));
    }

    TEST(ProgressTest, Interpolate_ReturnsTheStartAtZero)
    {
        EXPECT_EQ(interpolate(10, 20, Progress(0, 4)), 10);
    }

    TEST(ProgressTest, Interpolate_ReturnsTheEndAtOne)
    {
        EXPECT_EQ(interpolate(10, 20, Progress(4, 4)), 20);
    }

    TEST(ProgressTest, Interpolate_SplitsTheSpanExactly)
    {
        EXPECT_EQ(interpolate(10, 20, Progress(1, 4)), 12);
        EXPECT_EQ(interpolate(10, 20, Progress(2, 4)), 15);
        EXPECT_EQ(interpolate(10, 20, Progress(3, 4)), 17);
    }

    TEST(ProgressTest, Interpolate_TruncatesTowardsZeroGoingBackwards)
    {
        EXPECT_EQ(interpolate(0, -10, Progress(1, 4)), -2);
        EXPECT_EQ(interpolate(0, -10, Progress(3, 4)), -7);
    }

    TEST(ProgressTest, Interpolate_HandlesAZeroLengthSpan)
    {
        EXPECT_EQ(interpolate(7, 7, Progress(1, 3)), 7);
    }

    TEST(ProgressTest, Interpolate_WorksOnLargeCoordinates)
    {
        constexpr std::int64_t from = -1'000'000;
        constexpr std::int64_t to = 1'000'000;

        EXPECT_EQ(interpolate(from, to, Progress(1, 2)), 0);
    }

} // namespace antwika::animation
