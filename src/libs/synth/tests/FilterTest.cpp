#include "antwika/synth/Filter.hpp"

#include <gtest/gtest.h>

using antwika::synth::FilterCoefficients;
using antwika::synth::filterCoefficientsFor;
using antwika::synth::FilterDesc;
using antwika::synth::FilterMode;
using antwika::synth::filterSample;
using antwika::synth::FilterState;
using antwika::synth::kFilterModeCount;

namespace
{
    constexpr antwika::sound::SampleRate kRate = 48000;
    constexpr double kTolerance = 1e-6;

    constexpr FilterDesc kLowPass{
        .mode = FilterMode::LowPass, .cutoff = 1000.0, .resonance = 1.0};

    // Feeds a steady level in and returns what came out last.
    [[nodiscard]] float settled(
        FilterMode mode, const FilterDesc &desc, int samples)
    {
        const auto coefficients = filterCoefficientsFor(desc, kRate);
        FilterState state;
        float out = 0.0F;

        for (int sample = 0; sample < samples; ++sample)
        {
            out = filterSample(mode, coefficients, state, 1.0F);
        }

        return out;
    }
} // namespace

TEST(FilterTest, CountMatchesTheLastEnumerator)
{
    EXPECT_EQ(kFilterModeCount, 4U);
}

TEST(FilterTest, NoFilterHasNoCoefficients)
{
    const auto coefficients =
        filterCoefficientsFor(FilterDesc{}, kRate);

    EXPECT_EQ(coefficients, FilterCoefficients{});
}

TEST(FilterTest, CoefficientsRiseWithTheCutoff)
{
    const auto low = filterCoefficientsFor(kLowPass, kRate);

    const auto higher = filterCoefficientsFor(
        FilterDesc{
            .mode = FilterMode::LowPass,
            .cutoff = 4000.0,
            .resonance = 1.0},
        kRate);

    EXPECT_GT(low.frequency, 0.0);
    EXPECT_GT(higher.frequency, low.frequency);
    EXPECT_DOUBLE_EQ(low.damping, 1.0);
}

// A Chamberlin filter runs away above a sixth of the rate.
// The coefficient stops there however it was described.
TEST(FilterTest, CoefficientsClampWellBelowTheRate)
{
    const auto coefficients = filterCoefficientsFor(
        FilterDesc{
            .mode = FilterMode::LowPass,
            .cutoff = 40000.0,
            .resonance = 1.0},
        kRate);

    EXPECT_NEAR(coefficients.frequency, 1.0, kTolerance);
}

// Stability binds the coefficient pair: f*f + 2*f*q < 4.
// The old clamp bounded the frequency alone.
// A resonance past the pair's bound grew without limit.
TEST(FilterTest, DampingIsHeldInsideTheStabilityBound)
{
    const auto clamped = filterCoefficientsFor(
        FilterDesc{
            .mode = FilterMode::LowPass,
            .cutoff = 8000.0,
            .resonance = 2.0},
        kRate);

    // At the ratio cap the frequency coefficient is exactly one.
    // So the pair is stable only below three halves.
    EXPECT_LT(clamped.damping, 1.5);

    const auto kept = filterCoefficientsFor(
        FilterDesc{
            .mode = FilterMode::LowPass,
            .cutoff = 8000.0,
            .resonance = 1.0},
        kRate);

    EXPECT_DOUBLE_EQ(kept.damping, 1.0);
}

TEST(FilterTest, AHighResonanceImpulseStaysBounded)
{
    const auto coefficients = filterCoefficientsFor(
        FilterDesc{
            .mode = FilterMode::LowPass,
            .cutoff = 8000.0,
            .resonance = 2.0},
        kRate);

    FilterState state;

    auto out = filterSample(
        FilterMode::LowPass, coefficients, state, 1.0F);

    for (int sample = 0; sample < 4096; ++sample)
    {
        out = filterSample(
            FilterMode::LowPass, coefficients, state, 0.0F);

        ASSERT_LT(out, 10.0F);
        ASSERT_GT(out, -10.0F);
    }
}

TEST(FilterTest, NoneHandsTheSampleBackUntouched)
{
    const auto coefficients = filterCoefficientsFor(kLowPass, kRate);
    FilterState state;

    EXPECT_FLOAT_EQ(
        filterSample(FilterMode::None, coefficients, state, 0.375F),
        0.375F);

    // Untouched means the filter did not integrate either.
    EXPECT_EQ(state, FilterState{});
}

TEST(FilterTest, LowPassLetsASteadyLevelThroughEventually)
{
    EXPECT_LT(settled(FilterMode::LowPass, kLowPass, 1), 0.5F);
    EXPECT_GT(settled(FilterMode::LowPass, kLowPass, 4096), 0.5F);
}

TEST(FilterTest, HighPassBlocksASteadyLevelEventually)
{
    EXPECT_NEAR(settled(FilterMode::HighPass, kLowPass, 1), 1.0F, 1e-5F);
    EXPECT_LT(settled(FilterMode::HighPass, kLowPass, 4096), 0.5F);
}

TEST(FilterTest, BandPassPassesTheTurnoverAndSettlesAway)
{
    EXPECT_GT(settled(FilterMode::BandPass, kLowPass, 1), 0.0F);
    EXPECT_LT(settled(FilterMode::BandPass, kLowPass, 4096), 0.5F);
}

// Only a caller casting an integer in reaches this.
// Nothing on the render path may throw, so it hands the sample back.
TEST(FilterTest, AModeNoEnumeratorHasHandsTheSampleBack)
{
    const auto coefficients = filterCoefficientsFor(kLowPass, kRate);
    FilterState state;

    EXPECT_FLOAT_EQ(
        filterSample(
            static_cast<FilterMode>(77), coefficients, state, 0.5F),
        0.5F);
}

TEST(FilterTest, ValuesCompareFieldByField)
{
    EXPECT_EQ(kLowPass, kLowPass);
    EXPECT_NE(kLowPass, FilterDesc{});

    auto described = kLowPass;
    described.cutoff = 2000.0;
    EXPECT_NE(kLowPass, described);

    described = kLowPass;
    described.resonance = 0.5;
    EXPECT_NE(kLowPass, described);

    constexpr FilterState raised{.low = 1.0, .band = 0.5};
    constexpr FilterCoefficients tuned{.frequency = 1.0, .damping = 0.5};

    EXPECT_EQ(FilterState{}, FilterState{});
    EXPECT_NE(raised, FilterState{});

    auto moved = raised;
    moved.band = 0.25;
    EXPECT_NE(raised, moved);

    EXPECT_EQ(FilterCoefficients{}, FilterCoefficients{});
    EXPECT_NE(tuned, FilterCoefficients{});

    auto damped = tuned;
    damped.damping = 0.25;
    EXPECT_NE(tuned, damped);
}
