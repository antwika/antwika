#include <gtest/gtest.h>

#include <cstdint>

#include <antwika/animation/Progress.hpp>

#include "antwika/tween/Tween.hpp"
#include "antwika/tween/Ease.hpp"
#include "antwika/tween/Easing.hpp"
#include "antwika/tween/TweenError.hpp"

using antwika::animation::interpolate;
using antwika::animation::Progress;
using antwika::tween::ease;
using antwika::tween::Easing;
using antwika::tween::TweenError;
using antwika::tween::tweenBetween;

TEST(TweenTest, TweenBetween_LandsOnBothEndsExactly)
{
    EXPECT_EQ(
        tweenBetween(100, 200, Easing::CubicInOut, Progress(0, 8)), 100);
    EXPECT_EQ(
        tweenBetween(100, 200, Easing::CubicInOut, Progress(8, 8)), 200);
}

TEST(TweenTest, TweenBetween_ShapesTheSpanByTheCurve)
{
    EXPECT_EQ(
        tweenBetween(0, 100, Easing::CubicIn, Progress(1, 4)), 1);
    EXPECT_EQ(
        tweenBetween(0, 100, Easing::Linear, Progress(1, 4)), 25);
}

TEST(TweenTest, TweenBetween_WalksBackwardsWhenTheSpanDoes)
{
    EXPECT_EQ(
        tweenBetween(200, 100, Easing::Linear, Progress(1, 4)), 175);
    EXPECT_EQ(
        tweenBetween(200, 100, Easing::QuadIn, Progress(1, 2)), 175);
}

TEST(TweenTest, TweenBetween_CrossesZeroWithoutChangingShape)
{
    EXPECT_EQ(
        tweenBetween(-100, 100, Easing::Linear, Progress(1, 4)), -50);
}

TEST(TweenTest, TweenBetween_IsInterpolateOverEase)
{
    constexpr std::int64_t kFrom = -37;
    constexpr std::int64_t kTo = 419;

    ASSERT_EQ(
        tweenBetween(kFrom, kTo, Easing::QuartOut, Progress(4, 8)), 390);

    for (std::uint64_t step = 0; step <= 8; ++step)
    {
        const Progress at(step, 8);

        EXPECT_EQ(
            tweenBetween(kFrom, kTo, Easing::QuartOut, at),
            interpolate(kFrom, kTo, ease(Easing::QuartOut, at)))
            << step;
    }
}

TEST(TweenTest, TweenBetween_RefusesAnUnsignedOnlyDenominator)
{
    EXPECT_THROW(
        static_cast<void>(tweenBetween(
            0, 2, Easing::QuintInOut, Progress(3000, 7000))),
        TweenError);
}

TEST(TweenTest, TweenBetween_EasesTheLargestSignedFifthPower)
{
    EXPECT_EQ(
        tweenBetween(0, 2, Easing::QuintInOut, Progress(3104, 6208)), 1);
}

TEST(TweenTest, TweenBetween_CarriesTheEasingsRefusalThrough)
{
    EXPECT_THROW(
        static_cast<void>(tweenBetween(
            0, 10, static_cast<Easing>(std::uint8_t{200}),
            Progress(1, 2))),
        TweenError);
}
