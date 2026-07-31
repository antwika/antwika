#include "antwika/sound/WaveFormat.hpp"

#include <gtest/gtest.h>

using antwika::sound::kMaxChannels;
using antwika::sound::kStereo;
using antwika::sound::WaveFormat;

TEST(WaveFormatTest, DefaultsToStereoAtTheDefaultRate)
{
    constexpr WaveFormat format;

    EXPECT_EQ(format.rate, antwika::sound::kDefaultSampleRate);
    EXPECT_EQ(format.channels, kStereo);
}

TEST(WaveFormatTest, IsValid_TakesAnythingAudioCouldBe)
{
    EXPECT_TRUE((WaveFormat{.rate = 8000, .channels = 1}).isValid());
    EXPECT_TRUE(
        (WaveFormat{.rate = 192000, .channels = kMaxChannels}).isValid());
}

TEST(WaveFormatTest, IsValid_RefusesARateOfZero)
{
    EXPECT_FALSE((WaveFormat{.rate = 0, .channels = 2}).isValid());
}

TEST(WaveFormatTest, IsValid_RefusesNoChannelsAndTooMany)
{
    EXPECT_FALSE((WaveFormat{.rate = 48000, .channels = 0}).isValid());
    EXPECT_FALSE(
        (WaveFormat{.rate = 48000, .channels = kMaxChannels + 1})
            .isValid());
}

TEST(WaveFormatTest, EqualityComparesBothFields)
{
    constexpr WaveFormat format{.rate = 44100, .channels = 1};

    EXPECT_EQ(format, (WaveFormat{.rate = 44100, .channels = 1}));
    EXPECT_NE(format, (WaveFormat{.rate = 48000, .channels = 1}));
    EXPECT_NE(format, (WaveFormat{.rate = 44100, .channels = 2}));
}
