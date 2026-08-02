#include "antwika/tween/Ease.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>

#include <gtest/gtest.h>

#include <antwika/animation/Progress.hpp>
#include <antwika/time/Tick.hpp>

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
    // A denominator every bounce threshold divides exactly.
    // Eleven and four both go into it.
    // So a region boundary is a whole numerator.
    constexpr Tick kBounceSpan = 44;

    // 64 * 44 * 44, which is what bounceOut() puts every piece over.
    constexpr Tick kBounceWhole = 123904;
} // namespace

// Every curve is anchored at both ends.
// A span's two ends have to be exactly its two ends.
// Otherwise a walker would arrive somewhere it was not walking to.
TEST(EaseTest, EveryEasingStartsAtZeroAndEndsAtOne)
{
    for (std::size_t index = 0; index < kEasingCount; ++index)
    {
        const auto easing = static_cast<Easing>(index);

        const auto start = ease(easing, Progress(0, 4));
        EXPECT_EQ(start.numerator(), 0U) << "at index " << index;

        const auto end = ease(easing, Progress(4, 4));
        EXPECT_EQ(end.numerator(), end.denominator())
            << "at index " << index;
    }
}

// The library holds nothing between calls.
// So the same question has to come back with the same answer.
// That is what lets a replay redraw a frame rather than remember it.
TEST(EaseTest, AskingTwiceAnswersTheSame)
{
    for (std::size_t index = 0; index < kEasingCount; ++index)
    {
        const auto easing = static_cast<Easing>(index);

        EXPECT_EQ(
            ease(easing, Progress(13, kBounceSpan)),
            ease(easing, Progress(13, kBounceSpan)))
            << "at index " << index;
    }
}

// No curve here overshoots, which is why back and elastic are absent.
// A Progress could not hold one -- see Easing.hpp.
TEST(EaseTest, NoEasingLeavesTheUnitRange)
{
    for (std::size_t index = 0; index < kEasingCount; ++index)
    {
        const auto easing = static_cast<Easing>(index);

        for (Tick step = 0; step <= kBounceSpan; ++step)
        {
            const auto eased = ease(easing, Progress(step, kBounceSpan));

            EXPECT_LE(eased.numerator(), eased.denominator())
                << "index " << index << " at " << step;
        }
    }
}

TEST(EaseTest, LinearIsTheFractionItWasGiven)
{
    EXPECT_EQ(ease(Easing::Linear, Progress(1, 4)), Progress(1, 4));
    EXPECT_EQ(ease(Easing::Linear, Progress(3, 8)), Progress(3, 8));
}

TEST(EaseTest, RaisesTheFractionToTheCurvesPower)
{
    EXPECT_EQ(ease(Easing::QuadIn, Progress(1, 4)), Progress(1, 16));
    EXPECT_EQ(ease(Easing::CubicIn, Progress(1, 4)), Progress(1, 64));
    EXPECT_EQ(ease(Easing::QuartIn, Progress(1, 4)), Progress(1, 256));
    EXPECT_EQ(ease(Easing::QuintIn, Progress(1, 4)), Progress(1, 1024));
}

TEST(EaseTest, AnOutCurveIsItsInCurveReadBackwards)
{
    EXPECT_EQ(ease(Easing::QuadOut, Progress(1, 4)), Progress(7, 16));
    EXPECT_EQ(ease(Easing::CubicOut, Progress(1, 4)), Progress(37, 64));
    EXPECT_EQ(ease(Easing::QuartOut, Progress(1, 4)), Progress(175, 256));
    EXPECT_EQ(
        ease(Easing::QuintOut, Progress(1, 4)), Progress(781, 1024));
}

// Below the midpoint an inOut curve is its in curve, scaled.
TEST(EaseTest, AnInOutCurveEasesInBelowTheMidpoint)
{
    EXPECT_EQ(ease(Easing::QuadInOut, Progress(1, 4)), Progress(2, 16));
    EXPECT_EQ(ease(Easing::CubicInOut, Progress(1, 4)), Progress(4, 64));
    EXPECT_EQ(ease(Easing::QuartInOut, Progress(1, 4)), Progress(8, 256));
    EXPECT_EQ(
        ease(Easing::QuintInOut, Progress(1, 4)), Progress(16, 1024));
}

// And above it, its out curve.
TEST(EaseTest, AnInOutCurveEasesOutAboveTheMidpoint)
{
    EXPECT_EQ(ease(Easing::QuadInOut, Progress(3, 4)), Progress(14, 16));
    EXPECT_EQ(ease(Easing::CubicInOut, Progress(3, 4)), Progress(60, 64));
    EXPECT_EQ(
        ease(Easing::QuartInOut, Progress(3, 4)), Progress(248, 256));
    EXPECT_EQ(
        ease(Easing::QuintInOut, Progress(3, 4)), Progress(1008, 1024));
}

// The two halves have to meet, or the curve would jump at its middle.
TEST(EaseTest, AnInOutCurveIsAHalfAtItsMidpoint)
{
    EXPECT_EQ(ease(Easing::QuadInOut, Progress(2, 4)), Progress(8, 16));
    EXPECT_EQ(ease(Easing::CubicInOut, Progress(2, 4)), Progress(32, 64));
    EXPECT_EQ(
        ease(Easing::QuartInOut, Progress(2, 4)), Progress(128, 256));
    EXPECT_EQ(
        ease(Easing::QuintInOut, Progress(2, 4)), Progress(512, 1024));
}

// A bounce is four parabolas, each reached by its own numerator.
// The values are the standard ones.
// The curve touches one at 4/11, 8/11 and 10/11.
// It rests at its three vertices of 3/4, 15/16 and 63/64.
TEST(EaseTest, BounceOutAnswersInFourPieces)
{
    // The first rise, which reaches one at four elevenths.
    EXPECT_EQ(
        ease(Easing::BounceOut, Progress(8, kBounceSpan)),
        Progress(30976, kBounceWhole));
    EXPECT_EQ(
        ease(Easing::BounceOut, Progress(16, kBounceSpan)),
        Progress(kBounceWhole, kBounceWhole));

    // The second piece, on both sides of its vertex at six elevenths.
    EXPECT_EQ(
        ease(Easing::BounceOut, Progress(20, kBounceSpan)),
        Progress(100672, kBounceWhole));
    EXPECT_EQ(
        ease(Easing::BounceOut, Progress(28, kBounceSpan)),
        Progress(100672, kBounceWhole));

    // The third, at its vertex of fifteen sixteenths and beside it.
    EXPECT_EQ(
        ease(Easing::BounceOut, Progress(36, kBounceSpan)),
        Progress(116160, kBounceWhole));
    EXPECT_EQ(
        ease(Easing::BounceOut, Progress(34, kBounceSpan)),
        Progress(118096, kBounceWhole));

    // The fourth, at its vertex of sixty-three sixty-fourths.
    EXPECT_EQ(
        ease(Easing::BounceOut, Progress(42, kBounceSpan)),
        Progress(121968, kBounceWhole));
    EXPECT_EQ(
        ease(Easing::BounceOut, Progress(40, kBounceSpan)),
        Progress(kBounceWhole, kBounceWhole));
}

TEST(EaseTest, BounceInIsBounceOutReadBackwards)
{
    const auto forward = ease(Easing::BounceOut, Progress(8, kBounceSpan));
    const auto backward =
        ease(Easing::BounceIn, Progress(kBounceSpan - 8, kBounceSpan));

    EXPECT_EQ(
        backward.numerator(),
        forward.denominator() - forward.numerator());
    EXPECT_EQ(backward.denominator(), forward.denominator());
}

TEST(EaseTest, BounceInOutBouncesAtBothEnds)
{
    // Below the midpoint it is half of bounceIn.
    EXPECT_EQ(
        ease(Easing::BounceInOut, Progress(10, kBounceSpan)),
        Progress(30976, 2 * kBounceWhole));

    // At and above it, half of bounceOut lifted by a half.
    EXPECT_EQ(
        ease(Easing::BounceInOut, Progress(30, kBounceSpan)),
        Progress(2 * kBounceWhole, 2 * kBounceWhole));

    // The two halves meet, as an inOut curve's always must.
    const auto middle =
        ease(Easing::BounceInOut, Progress(22, kBounceSpan));
    EXPECT_EQ(middle.numerator() * 2, middle.denominator());
}

// Easing is a std::uint8_t, so this is a value a caller can produce.
// Refused rather than indexed with, which would read off the table.
TEST(EaseTest, RefusesAValueNoEnumeratorHas)
{
    EXPECT_THROW(
        static_cast<void>(
            ease(static_cast<Easing>(std::uint8_t{200}), Progress(1, 2))),
        TweenError);
}

// Shaping raises the denominator to the curve's power.
// So a big enough denominator runs out of room.
// Refused rather than wrapped, which is what TweenError is for.
TEST(EaseTest, RefusesADenominatorTooLargeForTheCurve)
{
    constexpr Tick kHuge = std::numeric_limits<Tick>::max();

    EXPECT_THROW(
        static_cast<void>(ease(Easing::QuintIn, Progress(kHuge, kHuge))),
        TweenError);
}

// The bounce puts its answer over 64 d^2 and then doubles it.
// So it runs out one squaring earlier than the polynomials do.
// The lift's addition is where a bounce sums two near-max terms.
// The signed bound is what that sum must not leave.
TEST(EaseTest, RefusesABounceWhoseLiftWouldNotFit)
{
    constexpr Tick kWide = 300000000;

    EXPECT_THROW(
        static_cast<void>(ease(
            Easing::BounceInOut, Progress(kWide - 1, kWide))),
        TweenError);
}

TEST(EaseTest, RefusesABounceWhoseHalvingWouldNotFit)
{
    constexpr Tick kWide = 500000000;

    EXPECT_THROW(
        static_cast<void>(
            ease(Easing::BounceInOut, Progress(kWide, kWide))),
        TweenError);
}

// A zero numerator takes a different path through the arithmetic.
// It is the case a walker sits on every time it starts a step.
TEST(EaseTest, RaisingZeroToAPowerIsZero)
{
    EXPECT_EQ(ease(Easing::QuadIn, Progress(0, 4)), Progress(0, 16));
    EXPECT_EQ(ease(Easing::QuintIn, Progress(0, 4)), Progress(0, 1024));
}
