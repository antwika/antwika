#include "antwika/sound/Waveform.hpp"

#include <gtest/gtest.h>

using antwika::sound::WaveFormat;
using antwika::sound::Waveform;

TEST(WaveformTest, AFreshWaveformHoldsNothing)
{
    const Waveform waveform;

    EXPECT_EQ(waveform.frameCount(), 0U);
    EXPECT_TRUE(waveform.isComplete());
}

TEST(WaveformTest, FrameCount_DividesTheSamplesByTheChannels)
{
    const Waveform waveform{
        .format = WaveFormat{.rate = 48000, .channels = 2},
        .samples = {0.0F, 0.1F, 0.2F, 0.3F, 0.4F, 0.5F}};

    EXPECT_EQ(waveform.frameCount(), 3U);
}

TEST(WaveformTest, FrameCount_ReportsNothingForAFormatWithNoChannels)
{
    const Waveform waveform{
        .format = WaveFormat{.rate = 48000, .channels = 0},
        .samples = {0.0F, 0.1F}};

    EXPECT_EQ(waveform.frameCount(), 0U);
}

TEST(WaveformTest, IsComplete_RefusesAPartialFrame)
{
    const Waveform waveform{
        .format = WaveFormat{.rate = 48000, .channels = 2},
        .samples = {0.0F, 0.1F, 0.2F}};

    EXPECT_FALSE(waveform.isComplete());
}

TEST(WaveformTest, IsComplete_RefusesAFormatThatIsNotOne)
{
    const Waveform waveform{
        .format = WaveFormat{.rate = 0, .channels = 2}, .samples = {}};

    EXPECT_FALSE(waveform.isComplete());
}

TEST(WaveformTest, EqualityComparesTheFormatAndEverySample)
{
    const Waveform waveform{
        .format = WaveFormat{.rate = 48000, .channels = 1},
        .samples = {0.5F}};

    EXPECT_EQ(waveform, waveform);

    auto retuned = waveform;
    retuned.format.rate = 44100;
    EXPECT_NE(waveform, retuned);

    auto quieter = waveform;
    quieter.samples[0] = 0.25F;
    EXPECT_NE(waveform, quieter);
}
