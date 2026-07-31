#include "antwika/sound/WavReader.hpp"

#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "antwika/sound/SoundError.hpp"
#include "antwika/sound/WaveFormat.hpp"

using antwika::sound::SoundError;
using antwika::sound::WaveFormat;
using antwika::sound::Waveform;
using antwika::sound::WavReader;

namespace
{
    // Every fixture here is bytes in memory rather than a file.
    // That is the whole reason the reader takes a stream.
    // Every refusal below is reachable without anything on disk.
    class Bytes
    {
    public:
        void tag(const std::string &text)
        {
            for (const auto letter : text)
            {
                data.push_back(static_cast<std::uint8_t>(letter));
            }
        }

        void u32(std::uint32_t value)
        {
            for (int shift = 0; shift < 32; shift += 8)
            {
                data.push_back(
                    static_cast<std::uint8_t>((value >> shift) & 0xFFU));
            }
        }

        void u16(std::uint16_t value)
        {
            data.push_back(static_cast<std::uint8_t>(value & 0xFFU));
            data.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xFFU));
        }

        void raw(std::uint8_t value)
        {
            data.push_back(value);
        }

        [[nodiscard]] std::string str() const
        {
            return std::string(data.begin(), data.end());
        }

        std::vector<std::uint8_t> data;
    };

    struct Wav
    {
        std::uint16_t encoding = 1;
        std::uint16_t channels = 1;
        std::uint32_t rate = 48000;
        std::uint16_t bits = 16;
        std::vector<std::uint8_t> samples;
        bool withFmt = true;
        bool withData = true;
        std::uint16_t fmtSize = 16;
    };

    [[nodiscard]] std::string build(const Wav &wav)
    {
        Bytes body;

        if (wav.withFmt)
        {
            body.tag("fmt ");
            body.u32(wav.fmtSize);
            body.u16(wav.encoding);
            body.u16(wav.channels);
            body.u32(wav.rate);
            body.u32(0); // byte rate, which this decoder does not read
            body.u16(0); // block align, likewise
            body.u16(wav.bits);

            for (std::uint16_t extra = 16; extra < wav.fmtSize; ++extra)
            {
                // An extensible sub-format GUID opens with the encoding.
                // So byte 24 of the chunk is the one that carries it.
                body.raw(extra == 24 ? 1 : 0);
            }
        }

        if (wav.withData)
        {
            body.tag("data");
            body.u32(static_cast<std::uint32_t>(wav.samples.size()));

            for (const auto sample : wav.samples)
            {
                body.raw(sample);
            }
        }

        Bytes all;
        all.tag("RIFF");
        all.u32(static_cast<std::uint32_t>(4 + body.data.size()));
        all.tag("WAVE");

        for (const auto byte : body.data)
        {
            all.raw(byte);
        }

        return all.str();
    }

    [[nodiscard]] Waveform decode(const std::string &bytes)
    {
        std::istringstream in(bytes);
        return WavReader{}.read(in);
    }
} // namespace

TEST(WavReaderTest, ReadsSixteenBitMonoAudio)
{
    // Two frames: full scale positive, then silence.
    const auto wav = decode(build(
        Wav{.samples = {0x00, 0x40, 0x00, 0x00}}));

    EXPECT_EQ(wav.format, (WaveFormat{.rate = 48000, .channels = 1}));
    ASSERT_EQ(wav.frameCount(), 2U);
    EXPECT_NEAR(wav.samples[0], 0.5F, 0.001F);
    EXPECT_EQ(wav.samples[1], 0.0F);
}

TEST(WavReaderTest, ReadsEightBitAudioAsUnsigned)
{
    // Eight-bit WAV is the one width stored unsigned, so 128 is silence.
    const auto wav = decode(build(
        Wav{.bits = 8, .samples = {128, 255, 0}}));

    ASSERT_EQ(wav.frameCount(), 3U);
    EXPECT_EQ(wav.samples[0], 0.0F);
    EXPECT_GT(wav.samples[1], 0.9F);
    EXPECT_EQ(wav.samples[2], -1.0F);
}

TEST(WavReaderTest, ReadsTwentyFourBitAudio)
{
    const auto wav = decode(build(
        Wav{.bits = 24, .samples = {0x00, 0x00, 0x40, 0x00, 0x00, 0x00}}));

    ASSERT_EQ(wav.frameCount(), 2U);
    EXPECT_NEAR(wav.samples[0], 0.5F, 0.001F);
    EXPECT_EQ(wav.samples[1], 0.0F);
}

TEST(WavReaderTest, ReadsThirtyTwoBitAudio)
{
    const auto wav = decode(build(
        Wav{.bits = 32, .samples = {0x00, 0x00, 0x00, 0x40}}));

    ASSERT_EQ(wav.frameCount(), 1U);
    EXPECT_NEAR(wav.samples[0], 0.5F, 0.001F);
}

TEST(WavReaderTest, ReadsThirtyTwoBitFloatAudio)
{
    // 0.5F, little-endian.
    const auto wav = decode(build(
        Wav{.encoding = 3, .bits = 32, .samples = {0x00, 0x00, 0x00, 0x3F}}));

    ASSERT_EQ(wav.frameCount(), 1U);
    EXPECT_FLOAT_EQ(wav.samples[0], 0.5F);
}

TEST(WavReaderTest, ReadsAnExtensibleHeaderBySubFormat)
{
    const auto wav = decode(build(
        Wav{.encoding = 0xFFFE, .samples = {0x00, 0x40}, .fmtSize = 40}));

    ASSERT_EQ(wav.frameCount(), 1U);
    EXPECT_NEAR(wav.samples[0], 0.5F, 0.001F);
}

TEST(WavReaderTest, ReadsInterleavedStereo)
{
    const auto wav = decode(build(
        Wav{.channels = 2, .samples = {0x00, 0x40, 0x00, 0xC0}}));

    EXPECT_EQ(wav.format.channels, 2U);
    ASSERT_EQ(wav.frameCount(), 1U);
    EXPECT_GT(wav.samples[0], 0.0F);
    EXPECT_LT(wav.samples[1], 0.0F);
}

// A file padded to a block boundary is ordinary.
// Its audio is still perfectly good, so the ragged tail is dropped.
TEST(WavReaderTest, DropsATrailingPartialFrame)
{
    const auto wav = decode(build(
        Wav{.channels = 2, .samples = {0x00, 0x40, 0x00, 0xC0, 0x11, 0x11}}));

    EXPECT_EQ(wav.frameCount(), 1U);
    EXPECT_EQ(wav.samples.size(), 2U);
}

TEST(WavReaderTest, ReadsChunksInEitherOrder)
{
    Bytes all;
    all.tag("RIFF");
    all.u32(4 + 8 + 4 + 8 + 16);
    all.tag("WAVE");

    // Data before format, which the specification allows.
    all.tag("data");
    all.u32(4);
    all.raw(0x00);
    all.raw(0x40);
    all.raw(0x00);
    all.raw(0x00);

    all.tag("fmt ");
    all.u32(16);
    all.u16(1);
    all.u16(1);
    all.u32(48000);
    all.u32(0);
    all.u16(0);
    all.u16(16);

    EXPECT_EQ(decode(all.str()).frameCount(), 2U);
}

TEST(WavReaderTest, SkipsAChunkItDoesNotKnow)
{
    Bytes all;
    all.tag("RIFF");
    all.u32(4 + 8 + 3 + 1 + 8 + 16 + 8 + 2);
    all.tag("WAVE");

    // An odd-length chunk, which is padded to an even boundary.
    all.tag("LIST");
    all.u32(3);
    all.raw('a');
    all.raw('b');
    all.raw('c');
    all.raw(0);

    all.tag("fmt ");
    all.u32(16);
    all.u16(1);
    all.u16(1);
    all.u32(48000);
    all.u32(0);
    all.u16(0);
    all.u16(16);

    all.tag("data");
    all.u32(2);
    all.raw(0x00);
    all.raw(0x40);

    EXPECT_EQ(decode(all.str()).frameCount(), 1U);
}

TEST(WavReaderTest, RefusesAnEmptyStream)
{
    EXPECT_THROW((void)decode(""), SoundError);
}

TEST(WavReaderTest, RefusesSomethingThatIsNotRiff)
{
    EXPECT_THROW((void)decode("not a wav file at all"), SoundError);
}

TEST(WavReaderTest, RefusesRiffThatIsNotWave)
{
    Bytes all;
    all.tag("RIFF");
    all.u32(4);
    all.tag("AVI ");

    EXPECT_THROW((void)decode(all.str()), SoundError);
}

TEST(WavReaderTest, RefusesAChunkRunningPastTheEnd)
{
    Bytes all;
    all.tag("RIFF");
    all.u32(4 + 8);
    all.tag("WAVE");
    all.tag("data");
    all.u32(9999);

    EXPECT_THROW((void)decode(all.str()), SoundError);
}

TEST(WavReaderTest, RefusesAFileWithNoFormatChunk)
{
    EXPECT_THROW(
        (void)decode(build(Wav{.samples = {0, 0}, .withFmt = false})),
        SoundError);
}

TEST(WavReaderTest, RefusesAFileWithNoDataChunk)
{
    EXPECT_THROW(
        (void)decode(build(Wav{.samples = {}, .withData = false})),
        SoundError);
}

TEST(WavReaderTest, RefusesAFormatChunkTooShortToDescribeAnything)
{
    Bytes all;
    all.tag("RIFF");
    all.u32(4 + 8 + 4);
    all.tag("WAVE");
    all.tag("fmt ");
    all.u32(4);
    all.u32(0);

    EXPECT_THROW((void)decode(all.str()), SoundError);
}

TEST(WavReaderTest, RefusesAnExtensibleHeaderWithNoSubFormat)
{
    EXPECT_THROW(
        (void)decode(build(
            Wav{.encoding = 0xFFFE, .samples = {0, 0}, .fmtSize = 18})),
        SoundError);
}

TEST(WavReaderTest, RefusesACompressionItDoesNotDecode)
{
    // Six is A-law, which is a real WAVE encoding and not one of ours.
    EXPECT_THROW(
        (void)decode(build(Wav{.encoding = 6, .samples = {0, 0}})),
        SoundError);
}

TEST(WavReaderTest, RefusesAFloatFileThatIsNotThirtyTwoBit)
{
    EXPECT_THROW(
        (void)decode(build(
            Wav{.encoding = 3, .bits = 64, .samples = {0, 0}})),
        SoundError);
}

TEST(WavReaderTest, RefusesASampleWidthItDoesNotDecode)
{
    EXPECT_THROW(
        (void)decode(build(Wav{.bits = 12, .samples = {0, 0}})),
        SoundError);
}

TEST(WavReaderTest, RefusesASampleRateOfZero)
{
    EXPECT_THROW(
        (void)decode(build(Wav{.rate = 0, .samples = {0, 0}})),
        SoundError);
}

TEST(WavReaderTest, RefusesNoChannelsAndTooManyChannels)
{
    EXPECT_THROW(
        (void)decode(build(Wav{.channels = 0, .samples = {0, 0}})),
        SoundError);

    EXPECT_THROW(
        (void)decode(build(Wav{.channels = 99, .samples = {0, 0}})),
        SoundError);
}
