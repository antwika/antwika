#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <limits>

#include <antwika/animation/Progress.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/tween/Ease.hpp"
#include "antwika/tween/Easing.hpp"
#include "antwika/tween/TweenError.hpp"

using antwika::animation::Progress;
using antwika::time::Tick;
using antwika::tween::ease;
using antwika::tween::Easing;
using antwika::tween::kEasingCount;
using antwika::tween::TweenError;

namespace
{
    constexpr Tick kBounceSpan = 44;

    constexpr Tick kBounceWhole = 123904;
}

TEST(EaseTest, Ease_StartsAtZeroAndEndsAtOne)
{
    for (std::size_t index = 0; index < kEasingCount; ++index)
    {
        const auto easing = static_cast<Easing>(index);

        const auto start = ease(easing, Progress(0, 4));
        EXPECT_EQ(start.numerator(), 0U) << index;

        const auto end = ease(easing, Progress(4, 4));
        EXPECT_EQ(end.numerator(), end.denominator()) << index;
    }
}
TEST(EaseTest, Ease_StaysInTheUnitRange)
{
    for (std::size_t index = 0; index < kEasingCount; ++index)
    {
        const auto easing = static_cast<Easing>(index);

        for (Tick step = 0; step <= kBounceSpan; ++step)
        {
            const auto eased = ease(easing, Progress(step, kBounceSpan));

            EXPECT_LE(eased.numerator(), eased.denominator())
                << index << ' ' << step;
        }
    }
}

TEST(EaseTest, Ease_ReturnsTheFractionWhenLinear)
{
    EXPECT_EQ(ease(Easing::Linear, Progress(1, 4)), Progress(1, 4));
    EXPECT_EQ(ease(Easing::Linear, Progress(3, 8)), Progress(3, 8));
}

TEST(EaseTest, Ease_RaisesTheFractionToTheCurvesPower)
{
    EXPECT_EQ(ease(Easing::QuadIn, Progress(1, 4)), Progress(1, 16));
    EXPECT_EQ(ease(Easing::CubicIn, Progress(1, 4)), Progress(1, 64));
    EXPECT_EQ(ease(Easing::QuartIn, Progress(1, 4)), Progress(1, 256));
    EXPECT_EQ(ease(Easing::QuintIn, Progress(1, 4)), Progress(1, 1024));
}

TEST(EaseTest, Ease_ReadsAnOutCurveAsItsInCurveBackwards)
{
    EXPECT_EQ(ease(Easing::QuadOut, Progress(1, 4)), Progress(7, 16));
    EXPECT_EQ(ease(Easing::CubicOut, Progress(1, 4)), Progress(37, 64));
    EXPECT_EQ(ease(Easing::QuartOut, Progress(1, 4)), Progress(175, 256));
    EXPECT_EQ(
        ease(Easing::QuintOut, Progress(1, 4)), Progress(781, 1024));
}

TEST(EaseTest, Ease_EasesInBelowTheMidpoint)
{
    EXPECT_EQ(ease(Easing::QuadInOut, Progress(1, 4)), Progress(2, 16));
    EXPECT_EQ(ease(Easing::CubicInOut, Progress(1, 4)), Progress(4, 64));
    EXPECT_EQ(ease(Easing::QuartInOut, Progress(1, 4)), Progress(8, 256));
    EXPECT_EQ(
        ease(Easing::QuintInOut, Progress(1, 4)), Progress(16, 1024));
}

TEST(EaseTest, Ease_EasesOutAboveTheMidpoint)
{
    EXPECT_EQ(ease(Easing::QuadInOut, Progress(3, 4)), Progress(14, 16));
    EXPECT_EQ(ease(Easing::CubicInOut, Progress(3, 4)), Progress(60, 64));
    EXPECT_EQ(
        ease(Easing::QuartInOut, Progress(3, 4)), Progress(248, 256));
    EXPECT_EQ(
        ease(Easing::QuintInOut, Progress(3, 4)), Progress(1008, 1024));
}

TEST(EaseTest, Ease_IsAHalfAtTheMidpoint)
{
    EXPECT_EQ(ease(Easing::QuadInOut, Progress(2, 4)), Progress(8, 16));
    EXPECT_EQ(ease(Easing::CubicInOut, Progress(2, 4)), Progress(32, 64));
    EXPECT_EQ(
        ease(Easing::QuartInOut, Progress(2, 4)), Progress(128, 256));
    EXPECT_EQ(
        ease(Easing::QuintInOut, Progress(2, 4)), Progress(512, 1024));
}

TEST(EaseTest, Ease_AnswersBounceOutInFourPieces)
{
    EXPECT_EQ(
        ease(Easing::BounceOut, Progress(8, kBounceSpan)),
        Progress(30976, kBounceWhole));
    EXPECT_EQ(
        ease(Easing::BounceOut, Progress(16, kBounceSpan)),
        Progress(kBounceWhole, kBounceWhole));

    EXPECT_EQ(
        ease(Easing::BounceOut, Progress(20, kBounceSpan)),
        Progress(100672, kBounceWhole));
    EXPECT_EQ(
        ease(Easing::BounceOut, Progress(28, kBounceSpan)),
        Progress(100672, kBounceWhole));

    EXPECT_EQ(
        ease(Easing::BounceOut, Progress(36, kBounceSpan)),
        Progress(116160, kBounceWhole));
    EXPECT_EQ(
        ease(Easing::BounceOut, Progress(34, kBounceSpan)),
        Progress(118096, kBounceWhole));

    EXPECT_EQ(
        ease(Easing::BounceOut, Progress(42, kBounceSpan)),
        Progress(121968, kBounceWhole));
    EXPECT_EQ(
        ease(Easing::BounceOut, Progress(40, kBounceSpan)),
        Progress(kBounceWhole, kBounceWhole));
}

TEST(EaseTest, Ease_ReadsBounceInAsBounceOutBackwards)
{
    const auto forward = ease(Easing::BounceOut, Progress(8, kBounceSpan));
    const auto backward =
        ease(Easing::BounceIn, Progress(kBounceSpan - 8, kBounceSpan));

    ASSERT_EQ(forward, Progress(30976, kBounceWhole));

    EXPECT_EQ(
        backward.numerator(),
        forward.denominator() - forward.numerator());
    EXPECT_EQ(backward.denominator(), forward.denominator());
}

TEST(EaseTest, Ease_BouncesAtBothEndsForBounceInOut)
{
    EXPECT_EQ(
        ease(Easing::BounceInOut, Progress(10, kBounceSpan)),
        Progress(30976, 2 * kBounceWhole));

    EXPECT_EQ(
        ease(Easing::BounceInOut, Progress(30, kBounceSpan)),
        Progress(2 * kBounceWhole, 2 * kBounceWhole));

    const auto middle =
        ease(Easing::BounceInOut, Progress(22, kBounceSpan));
    EXPECT_EQ(middle.numerator() * 2, middle.denominator());
}

TEST(EaseTest, Ease_RefusesAValueNoEnumeratorHas)
{
    EXPECT_THROW(
        static_cast<void>(
            ease(static_cast<Easing>(std::uint8_t{200}), Progress(1, 2))),
        TweenError);
}

TEST(EaseTest, Ease_RefusesTheValueOnePastTheLastEasing)
{
    EXPECT_THROW(
        static_cast<void>(ease(
            static_cast<Easing>(static_cast<std::uint8_t>(kEasingCount)),
            Progress(1, 2))),
        TweenError);
}

TEST(EaseTest, Ease_AcceptsTheLargestNumbersThatStillFit)
{
    constexpr Tick kLargest = static_cast<Tick>(
        std::numeric_limits<std::int64_t>::max());

    EXPECT_EQ(
        ease(Easing::Linear, Progress(kLargest, kLargest)),
        Progress(kLargest, kLargest));
}

TEST(EaseTest, Ease_RefusesATooLargeDenominator)
{
    constexpr Tick kHuge = std::numeric_limits<Tick>::max();

    EXPECT_THROW(
        static_cast<void>(ease(Easing::QuintIn, Progress(kHuge, kHuge))),
        TweenError);
}

TEST(EaseTest, Ease_RefusesABounceWhoseLiftWouldNotFit)
{
    constexpr Tick kWide = 300000000;

    EXPECT_THROW(
        static_cast<void>(ease(
            Easing::BounceInOut, Progress(kWide - 1, kWide))),
        TweenError);
}

TEST(EaseTest, Ease_RefusesABounceWhoseHalvingWouldNotFit)
{
    constexpr Tick kWide = 500000000;

    EXPECT_THROW(
        static_cast<void>(
            ease(Easing::BounceInOut, Progress(kWide, kWide))),
        TweenError);
}

TEST(EaseTest, Ease_RaisesZeroToAPowerAsZero)
{
    EXPECT_EQ(ease(Easing::QuadIn, Progress(0, 4)), Progress(0, 16));
    EXPECT_EQ(ease(Easing::QuintIn, Progress(0, 4)), Progress(0, 1024));
}
