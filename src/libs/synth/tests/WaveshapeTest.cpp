#include "antwika/synth/Waveshape.hpp"

#include <cstddef>
#include <set>
#include <string_view>

#include <gtest/gtest.h>

using antwika::synth::isPeriodic;
using antwika::synth::kWaveshapeCount;
using antwika::synth::Waveshape;
using antwika::synth::waveshapeIndex;
using antwika::synth::waveshapeName;

TEST(WaveshapeTest, CountMatchesTheLastEnumerator)
{
    EXPECT_EQ(kWaveshapeCount, 5U);
    EXPECT_EQ(waveshapeIndex(Waveshape::Sine), 0U);
    EXPECT_EQ(waveshapeIndex(Waveshape::Noise), kWaveshapeCount - 1);
}

TEST(WaveshapeTest, EveryShapeHasItsOwnName)
{
    std::set<std::string_view> seen;

    for (std::size_t index = 0; index < kWaveshapeCount; ++index)
    {
        const auto shape = static_cast<Waveshape>(index);
        const auto name = waveshapeName(shape);

        EXPECT_NE(name, "unknown") << "at index " << index;
        EXPECT_TRUE(seen.insert(name).second) << "repeated " << name;
    }

    EXPECT_EQ(seen.size(), kWaveshapeCount);
}

TEST(WaveshapeTest, NamesTheOnesACallerWritesDown)
{
    EXPECT_EQ(waveshapeName(Waveshape::Sine), "sine");
    EXPECT_EQ(waveshapeName(Waveshape::Saw), "saw");
    EXPECT_EQ(waveshapeName(Waveshape::Square), "square");
    EXPECT_EQ(waveshapeName(Waveshape::Triangle), "triangle");
    EXPECT_EQ(waveshapeName(Waveshape::Noise), "noise");
}

// A name is for a message somebody is already reading.
// One that threw would take the program with it.
TEST(WaveshapeTest, NamesAValueNoEnumeratorHasWithoutThrowing)
{
    EXPECT_EQ(waveshapeName(static_cast<Waveshape>(99)), "unknown");
}

// Noise is the one shape a frequency means nothing for.
// That is what the trigger path validates on.
TEST(WaveshapeTest, EveryShapeButNoiseRepeatsOverAPhase)
{
    EXPECT_TRUE(isPeriodic(Waveshape::Sine));
    EXPECT_TRUE(isPeriodic(Waveshape::Saw));
    EXPECT_TRUE(isPeriodic(Waveshape::Square));
    EXPECT_TRUE(isPeriodic(Waveshape::Triangle));
    EXPECT_FALSE(isPeriodic(Waveshape::Noise));
}
