#include <gtest/gtest.h>

#include <cstddef>
#include <set>
#include <string_view>

#include "antwika/synth/Waveshape.hpp"

using antwika::synth::isPeriodic;
using antwika::synth::kWaveshapeCount;
using antwika::synth::Waveshape;
using antwika::synth::waveshapeIndex;
using antwika::synth::waveshapeName;

TEST(WaveshapeTest, WaveshapeIndex_MatchesTheLastEnumerator)
{
    EXPECT_EQ(kWaveshapeCount, 5U);
    EXPECT_EQ(waveshapeIndex(Waveshape::Sine), 0U);
    EXPECT_EQ(waveshapeIndex(Waveshape::Noise), kWaveshapeCount - 1);
}

TEST(WaveshapeTest, WaveshapeName_IsUniquePerShape)
{
    std::set<std::string_view> seen;

    for (std::size_t index = 0; index < kWaveshapeCount; ++index)
    {
        const auto shape = static_cast<Waveshape>(index);
        const auto name = waveshapeName(shape);

        EXPECT_NE(name, "unknown") << index;
        EXPECT_TRUE(seen.insert(name).second) << name;
    }

    EXPECT_EQ(seen.size(), kWaveshapeCount);
}

TEST(WaveshapeTest, WaveshapeName_NamesTheCommonShapes)
{
    EXPECT_EQ(waveshapeName(Waveshape::Sine), "sine");
    EXPECT_EQ(waveshapeName(Waveshape::Saw), "saw");
    EXPECT_EQ(waveshapeName(Waveshape::Square), "square");
    EXPECT_EQ(waveshapeName(Waveshape::Triangle), "triangle");
    EXPECT_EQ(waveshapeName(Waveshape::Noise), "noise");
}

TEST(WaveshapeTest, WaveshapeName_NamesAnUnknownValueSafely)
{
    EXPECT_EQ(waveshapeName(static_cast<Waveshape>(99)), "unknown");
}

TEST(WaveshapeTest, WaveshapeName_StopsAtTheFirstValueOffTheEnd)
{
    EXPECT_EQ(
        waveshapeName(static_cast<Waveshape>(kWaveshapeCount)), "unknown");
}

TEST(WaveshapeTest, IsPeriodic_IsTrueForEveryShapeButNoise)
{
    EXPECT_TRUE(isPeriodic(Waveshape::Sine));
    EXPECT_TRUE(isPeriodic(Waveshape::Saw));
    EXPECT_TRUE(isPeriodic(Waveshape::Square));
    EXPECT_TRUE(isPeriodic(Waveshape::Triangle));
    EXPECT_FALSE(isPeriodic(Waveshape::Noise));
}
