#include "antwika/tween/Tween.hpp"

#include <cstdint>

#include <gtest/gtest.h>

#include <antwika/animation/Progress.hpp>

#include "antwika/tween/Ease.hpp"
#include "antwika/tween/Easing.hpp"
#include "antwika/tween/TweenError.hpp"

using antwika::animation::interpolate;
using antwika::animation::Progress;
using antwika::tween::ease;
using antwika::tween::Easing;
using antwika::tween::TweenError;
using antwika::tween::tweenBetween;

TEST(TweenTest, LandsOnBothEndsExactly)
{
    EXPECT_EQ(
        tweenBetween(100, 200, Easing::CubicInOut, Progress(0, 8)), 100);
    EXPECT_EQ(
        tweenBetween(100, 200, Easing::CubicInOut, Progress(8, 8)), 200);
}

TEST(TweenTest, ShapesTheSpanByTheCurve)
{
    // A quarter of the way along a hundred, eased in by a cubic.
    // That is a sixty-fourth of it rather than a quarter.
    EXPECT_EQ(
        tweenBetween(0, 100, Easing::CubicIn, Progress(1, 4)), 1);
    EXPECT_EQ(
        tweenBetween(0, 100, Easing::Linear, Progress(1, 4)), 25);
}

TEST(TweenTest, WalksBackwardsWhenTheSpanDoes)
{
    EXPECT_EQ(
        tweenBetween(200, 100, Easing::Linear, Progress(1, 4)), 175);
    EXPECT_EQ(
        tweenBetween(200, 100, Easing::QuadIn, Progress(1, 2)), 175);
}

TEST(TweenTest, CrossesZeroWithoutChangingShape)
{
    EXPECT_EQ(
        tweenBetween(-100, 100, Easing::Linear, Progress(1, 4)), -50);
}

// It exists to save a caller composing the two.
// So it had better be the composition it claims to be.
TEST(TweenTest, IsExactlyInterpolateOverEase)
{
    constexpr std::int64_t kFrom = -37;
    constexpr std::int64_t kTo = 419;

    for (std::uint64_t step = 0; step <= 8; ++step)
    {
        const Progress at(step, 8);

        EXPECT_EQ(
            tweenBetween(kFrom, kTo, Easing::QuartOut, at),
            interpolate(kFrom, kTo, ease(Easing::QuartOut, at)))
            << "at " << step;
    }
}

TEST(TweenTest, CarriesTheEasingsRefusalThrough)
{
    EXPECT_THROW(
        static_cast<void>(tweenBetween(
            0, 10, static_cast<Easing>(std::uint8_t{200}),
            Progress(1, 2))),
        TweenError);
}
