#include "antwika/synth/Oscillate.hpp"

#include <cstdint>
#include <set>

#include <gtest/gtest.h>

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
} // namespace

TEST(OscillateTest, SineStartsAtZeroAndPeaksAtAQuarter)
{
    EXPECT_NEAR(at(Waveshape::Sine, 0.0), 0.0F, kTolerance);
    EXPECT_NEAR(at(Waveshape::Sine, 0.25), 1.0F, kTolerance);
    EXPECT_NEAR(at(Waveshape::Sine, 0.5), 0.0F, kTolerance);
    EXPECT_NEAR(at(Waveshape::Sine, 0.75), -1.0F, kTolerance);
}

TEST(OscillateTest, SawRisesAcrossTheWholeCycle)
{
    EXPECT_NEAR(at(Waveshape::Saw, 0.0), -1.0F, kTolerance);
    EXPECT_NEAR(at(Waveshape::Saw, 0.5), 0.0F, kTolerance);
    EXPECT_NEAR(at(Waveshape::Saw, 0.75), 0.5F, kTolerance);
}

TEST(OscillateTest, SquareIsTwoLevelsAndNothingBetween)
{
    EXPECT_FLOAT_EQ(at(Waveshape::Square, 0.0), 1.0F);
    EXPECT_FLOAT_EQ(at(Waveshape::Square, 0.49), 1.0F);
    EXPECT_FLOAT_EQ(at(Waveshape::Square, 0.5), -1.0F);
    EXPECT_FLOAT_EQ(at(Waveshape::Square, 0.99), -1.0F);
}

TEST(OscillateTest, TriangleClimbsThenFalls)
{
    EXPECT_NEAR(at(Waveshape::Triangle, 0.0), -1.0F, kTolerance);
    EXPECT_NEAR(at(Waveshape::Triangle, 0.25), 0.0F, kTolerance);
    EXPECT_NEAR(at(Waveshape::Triangle, 0.5), 1.0F, kTolerance);
    EXPECT_NEAR(at(Waveshape::Triangle, 0.75), 0.0F, kTolerance);
}

// The property the whole design rests on.
// Noise is a function of where one voice has got to.
// So it does not depend on what else happens to be sounding.
TEST(OscillateTest, NoiseRepeatsForTheSameSeedAndPosition)
{
    for (std::uint64_t position = 0; position < 32; ++position)
    {
        EXPECT_FLOAT_EQ(
            oscillate(Waveshape::Noise, 0.0, 7, position),
            oscillate(Waveshape::Noise, 0.0, 7, position));
    }
}

TEST(OscillateTest, NoiseIgnoresThePhaseEntirely)
{
    EXPECT_FLOAT_EQ(
        oscillate(Waveshape::Noise, 0.0, 7, 3),
        oscillate(Waveshape::Noise, 0.9, 7, 3));
}

TEST(OscillateTest, NoiseDiffersBySeedAndByPosition)
{
    std::set<float> seen;

    for (std::uint64_t position = 0; position < 16; ++position)
    {
        seen.insert(oscillate(Waveshape::Noise, 0.0, 1, position));
    }

    // Sixteen draws landing on one value would be a bug.
    // The position would not be reaching the generator at all.
    EXPECT_GT(seen.size(), 8U);

    EXPECT_NE(
        oscillate(Waveshape::Noise, 0.0, 1, 5),
        oscillate(Waveshape::Noise, 0.0, 2, 5));
}

TEST(OscillateTest, NoiseStaysInRange)
{
    for (std::uint64_t position = 0; position < 512; ++position)
    {
        const auto sample =
            oscillate(Waveshape::Noise, 0.0, 99, position);

        EXPECT_GE(sample, -1.0F);
        EXPECT_LE(sample, 1.0F);
    }
}

// Nothing on the render path may throw, so an unnamed shape is silence.
TEST(OscillateTest, AShapeNoEnumeratorHasIsSilent)
{
    EXPECT_FLOAT_EQ(at(static_cast<Waveshape>(42), 0.25), 0.0F);
}
