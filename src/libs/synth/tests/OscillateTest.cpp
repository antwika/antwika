#include <gtest/gtest.h>

#include <cstdint>
#include <set>

#include "antwika/synth/Oscillate.hpp"
#include "antwika/synth/Waveshape.hpp"

using antwika::synth::oscillate;
using antwika::synth::Waveshape;

namespace
{
    constexpr float kTolerance = 1e-5F;

    [[nodiscard]] float at(Waveshape shape, double phase)
    {
        return oscillate(shape, phase, 0, 0);
    }
}

TEST(OscillateTest, Oscillate_StartsSineAtZeroAndPeaksAtAQuarter)
{
    EXPECT_NEAR(at(Waveshape::Sine, 0.0), 0.0F, kTolerance);
    EXPECT_NEAR(at(Waveshape::Sine, 0.25), 1.0F, kTolerance);
    EXPECT_NEAR(at(Waveshape::Sine, 0.5), 0.0F, kTolerance);
    EXPECT_NEAR(at(Waveshape::Sine, 0.75), -1.0F, kTolerance);
}

TEST(OscillateTest, Oscillate_RisesSawAcrossTheWholeCycle)
{
    EXPECT_NEAR(at(Waveshape::Saw, 0.0), -1.0F, kTolerance);
    EXPECT_NEAR(at(Waveshape::Saw, 0.5), 0.0F, kTolerance);
    EXPECT_NEAR(at(Waveshape::Saw, 0.75), 0.5F, kTolerance);
}

TEST(OscillateTest, Oscillate_GivesSquareTwoLevelsOnly)
{
    EXPECT_FLOAT_EQ(at(Waveshape::Square, 0.0), 1.0F);
    EXPECT_FLOAT_EQ(at(Waveshape::Square, 0.49), 1.0F);
    EXPECT_FLOAT_EQ(at(Waveshape::Square, 0.5), -1.0F);
    EXPECT_FLOAT_EQ(at(Waveshape::Square, 0.99), -1.0F);
}

TEST(OscillateTest, Oscillate_ClimbsThenFallsForTriangle)
{
    EXPECT_NEAR(at(Waveshape::Triangle, 0.0), -1.0F, kTolerance);
    EXPECT_NEAR(at(Waveshape::Triangle, 0.25), 0.0F, kTolerance);
    EXPECT_NEAR(at(Waveshape::Triangle, 0.5), 1.0F, kTolerance);
    EXPECT_NEAR(at(Waveshape::Triangle, 0.75), 0.0F, kTolerance);
}

TEST(OscillateTest, Oscillate_DrawsNoiseFromTheSeedAndPosition)
{
    EXPECT_FLOAT_EQ(oscillate(Waveshape::Noise, 0.0, 7, 0), -0.22034061F);
    EXPECT_FLOAT_EQ(oscillate(Waveshape::Noise, 0.0, 7, 1), 0.84740019F);
    EXPECT_FLOAT_EQ(oscillate(Waveshape::Noise, 0.0, 7, 3), -0.68119168F);
}

TEST(OscillateTest, Oscillate_RepeatsNoiseForASeedAndPosition)
{
    EXPECT_FLOAT_EQ(oscillate(Waveshape::Noise, 0.0, 7, 3), -0.68119168F);
    EXPECT_FLOAT_EQ(oscillate(Waveshape::Noise, 0.0, 7, 3), -0.68119168F);
}

TEST(OscillateTest, Oscillate_IgnoresThePhaseForNoise)
{
    EXPECT_FLOAT_EQ(
        oscillate(Waveshape::Noise, 0.0, 7, 3),
        oscillate(Waveshape::Noise, 0.9, 7, 3));
}

TEST(OscillateTest, Oscillate_VariesNoiseBySeedAndPosition)
{
    std::set<float> seen;

    for (std::uint64_t position = 0; position < 16; ++position)
    {
        seen.insert(oscillate(Waveshape::Noise, 0.0, 1, position));
    }

    EXPECT_GT(seen.size(), 8U);

    EXPECT_FLOAT_EQ(oscillate(Waveshape::Noise, 0.0, 1, 5), 0.64934313F);
    EXPECT_FLOAT_EQ(oscillate(Waveshape::Noise, 0.0, 2, 5), -0.30675554F);
}

TEST(OscillateTest, Oscillate_KeepsNoiseInRange)
{
    for (std::uint64_t position = 0; position < 512; ++position)
    {
        const auto sample =
            oscillate(Waveshape::Noise, 0.0, 99, position);

        EXPECT_GE(sample, -1.0F);
        EXPECT_LE(sample, 1.0F);
    }
}

TEST(OscillateTest, Oscillate_IsSilentForAnUnknownShape)
{
    EXPECT_FLOAT_EQ(at(static_cast<Waveshape>(42), 0.25), 0.0F);
}
