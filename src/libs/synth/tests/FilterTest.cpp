#include <gtest/gtest.h>

#include "antwika/synth/Filter.hpp"

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
}

TEST(FilterTest, FilterCount_MatchesTheLastEnumerator)
{
    EXPECT_EQ(kFilterModeCount, 4U);
}

TEST(FilterTest, FilterCoefficientsFor_GivesNoneNoCoefficients)
{
    const auto coefficients =
        filterCoefficientsFor(FilterDesc{}, kRate);

    EXPECT_EQ(coefficients, FilterCoefficients{});
}

TEST(FilterTest, FilterCoefficientsFor_RiseWithTheCutoff)
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

TEST(FilterTest, FilterCoefficientsFor_ClampBelowTheRate)
{
    const auto coefficients = filterCoefficientsFor(
        FilterDesc{
            .mode = FilterMode::LowPass,
            .cutoff = 40000.0,
            .resonance = 1.0},
        kRate);

    EXPECT_NEAR(coefficients.frequency, 1.0, kTolerance);
}

TEST(FilterTest, FilterCoefficientsFor_HoldDampingInBound)
{
    const auto clamped = filterCoefficientsFor(
        FilterDesc{
            .mode = FilterMode::LowPass,
            .cutoff = 8000.0,
            .resonance = 2.0},
        kRate);

    EXPECT_LT(clamped.damping, 1.5);

    const auto kept = filterCoefficientsFor(
        FilterDesc{
            .mode = FilterMode::LowPass,
            .cutoff = 8000.0,
            .resonance = 1.0},
        kRate);

    EXPECT_DOUBLE_EQ(kept.damping, 1.0);
}

TEST(FilterTest, FilterCoefficientsFor_RaiseTheCeilingAsTheCutoffFalls)
{
    const auto low = filterCoefficientsFor(
        FilterDesc{
            .mode = FilterMode::LowPass,
            .cutoff = 1000.0,
            .resonance = 20.0},
        kRate);

    EXPECT_NEAR(low.frequency, 0.13080626, 1e-6);
    EXPECT_NEAR(low.damping, 15.22340861, 1e-6);
}

TEST(FilterTest, FilterSample_StaysBoundedAtHighResonance)
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

TEST(FilterTest, FilterSample_HandsBackNoneUntouched)
{
    const auto coefficients = filterCoefficientsFor(kLowPass, kRate);
    FilterState state;

    EXPECT_FLOAT_EQ(
        filterSample(FilterMode::None, coefficients, state, 0.375F),
        0.375F);

    EXPECT_EQ(state, FilterState{});
}

TEST(FilterTest, FilterSample_LetsASteadyLevelThroughLowPass)
{
    EXPECT_LT(settled(FilterMode::LowPass, kLowPass, 1), 0.5F);
    EXPECT_GT(settled(FilterMode::LowPass, kLowPass, 4096), 0.5F);
}

TEST(FilterTest, FilterSample_BlocksASteadyLevelHighPass)
{
    EXPECT_NEAR(settled(FilterMode::HighPass, kLowPass, 1), 1.0F, 1e-5F);
    EXPECT_LT(settled(FilterMode::HighPass, kLowPass, 4096), 0.5F);
}

TEST(FilterTest, FilterSample_PassesTheTurnoverBandPass)
{
    EXPECT_GT(settled(FilterMode::BandPass, kLowPass, 1), 0.0F);
    EXPECT_LT(settled(FilterMode::BandPass, kLowPass, 4096), 0.5F);
}

TEST(FilterTest, FilterSample_HandsBackAnUnknownMode)
{
    const auto coefficients = filterCoefficientsFor(kLowPass, kRate);
    FilterState state;

    EXPECT_FLOAT_EQ(
        filterSample(
            static_cast<FilterMode>(77), coefficients, state, 0.5F),
        0.5F);
}

TEST(FilterTest, FilterDescEquals_ComparesFieldByField)
{
    const auto twin = kLowPass;
    EXPECT_EQ(kLowPass, twin);
    EXPECT_NE(kLowPass, FilterDesc{});

    auto described = kLowPass;
    described.mode = FilterMode::HighPass;
    EXPECT_NE(kLowPass, described);

    described = kLowPass;
    described.cutoff = 2000.0;
    EXPECT_NE(kLowPass, described);

    described = kLowPass;
    described.resonance = 0.5;
    EXPECT_NE(kLowPass, described);
}

TEST(FilterTest, FilterStateEquals_ComparesFieldByField)
{
    constexpr FilterState raised{.low = 1.0, .band = 0.5};

    const auto twin = raised;
    EXPECT_EQ(raised, twin);
    EXPECT_NE(raised, FilterState{});

    auto moved = raised;
    moved.low = 0.25;
    EXPECT_NE(raised, moved);

    moved = raised;
    moved.band = 0.25;
    EXPECT_NE(raised, moved);
}

TEST(FilterTest, FilterCoefficientsEquals_ComparesFieldByField)
{
    constexpr FilterCoefficients tuned{.frequency = 1.0, .damping = 0.5};

    const auto twin = tuned;
    EXPECT_EQ(tuned, twin);
    EXPECT_NE(tuned, FilterCoefficients{});

    auto damped = tuned;
    damped.frequency = 0.25;
    EXPECT_NE(tuned, damped);

    damped = tuned;
    damped.damping = 0.25;
    EXPECT_NE(tuned, damped);
}
