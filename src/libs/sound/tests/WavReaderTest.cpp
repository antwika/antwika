#include <gtest/gtest.h>

#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

#include "antwika/sound/WavReader.hpp"
#include "antwika/sound/SoundError.hpp"
#include "antwika/sound/WaveFormat.hpp"

using antwika::sound::SoundError;
using antwika::sound::WaveFormat;
using antwika::sound::Waveform;
using antwika::sound::WavReader;

namespace
{
    class Bytes final
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

    struct Wav final
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
        Bytes bodyBytes;

        if (wav.withFmt)
        {
            bodyBytes.tag("fmt ");
            bodyBytes.u32(wav.fmtSize);
            bodyBytes.u16(wav.encoding);
            bodyBytes.u16(wav.channels);
            bodyBytes.u32(wav.rate);
            bodyBytes.u32(0);
            bodyBytes.u16(0);
            bodyBytes.u16(wav.bits);

            for (std::uint16_t extra = 16; extra < wav.fmtSize; ++extra)
            {
                bodyBytes.raw(extra == 24 ? 1 : 0);
            }
        }

        if (wav.withData)
        {
            bodyBytes.tag("data");
            bodyBytes.u32(static_cast<std::uint32_t>(wav.samples.size()));

            for (const auto sample : wav.samples)
            {
                bodyBytes.raw(sample);
            }
        }

        Bytes allBytes;
        allBytes.tag("RIFF");
        allBytes.u32(static_cast<std::uint32_t>(4 + bodyBytes.data.size()));
        allBytes.tag("WAVE");

        for (const auto byte : bodyBytes.data)
        {
            allBytes.raw(byte);
        }

        return allBytes.str();
    }

    [[nodiscard]] Waveform decode(const std::string &bytes)
    {
        std::istringstream inputStream(bytes);
        return WavReader{}.read(inputStream);
    }
}

TEST(WavReaderTest, Decode_ReadsSixteenBitMonoAudio)
{
    const auto wav = decode(build(
        Wav{.samples = {0x00, 0x40, 0x00, 0x00}}));

    EXPECT_EQ(wav.format, (WaveFormat{.rate = 48000, .channels = 1}));
    ASSERT_EQ(wav.frameCount(), 2U);
    EXPECT_NEAR(wav.samples[0], 0.5F, 0.001F);
    EXPECT_EQ(wav.samples[1], 0.0F);
}

TEST(WavReaderTest, Decode_ReadsEightBitAudioAsUnsigned)
{
    const auto wav = decode(build(
        Wav{.bits = 8, .samples = {128, 255, 0}}));

    ASSERT_EQ(wav.frameCount(), 3U);
    EXPECT_EQ(wav.samples[0], 0.0F);
    EXPECT_GT(wav.samples[1], 0.9F);
    EXPECT_EQ(wav.samples[2], -1.0F);
}

TEST(WavReaderTest, Decode_ReadsTwentyFourBitAudio)
{
    const auto wav = decode(build(
        Wav{.bits = 24, .samples = {0x00, 0x00, 0x40, 0x00, 0x00, 0x00}}));

    ASSERT_EQ(wav.frameCount(), 2U);
    EXPECT_NEAR(wav.samples[0], 0.5F, 0.001F);
    EXPECT_EQ(wav.samples[1], 0.0F);
}

TEST(WavReaderTest, Decode_ReadsThirtyTwoBitAudio)
{
    const auto wav = decode(build(
        Wav{.bits = 32, .samples = {0x00, 0x00, 0x00, 0x40}}));

    ASSERT_EQ(wav.frameCount(), 1U);
    EXPECT_NEAR(wav.samples[0], 0.5F, 0.001F);
}

TEST(WavReaderTest, Decode_ReadsThirtyTwoBitFloatAudio)
{
    const auto wav = decode(build(
        Wav{.encoding = 3, .bits = 32, .samples = {0x00, 0x00, 0x00, 0x3F}}));

    ASSERT_EQ(wav.frameCount(), 1U);
    EXPECT_FLOAT_EQ(wav.samples[0], 0.5F);
}

TEST(WavReaderTest, Decode_ReadsAnExtensibleHeaderBySubFormat)
{
    const auto wav = decode(build(
        Wav{.encoding = 0xFFFE, .samples = {0x00, 0x40}, .fmtSize = 40}));

    ASSERT_EQ(wav.frameCount(), 1U);
    EXPECT_NEAR(wav.samples[0], 0.5F, 0.001F);
}

TEST(WavReaderTest, Decode_ReadsInterleavedStereo)
{
    const auto wav = decode(build(
        Wav{.channels = 2, .samples = {0x00, 0x40, 0x00, 0xC0}}));

    EXPECT_EQ(wav.format.channels, 2U);
    ASSERT_EQ(wav.frameCount(), 1U);
    EXPECT_GT(wav.samples[0], 0.0F);
    EXPECT_LT(wav.samples[1], 0.0F);
}

TEST(WavReaderTest, Decode_DropsATrailingPartialFrame)
{
    const auto wav = decode(build(
        Wav{.channels = 2, .samples = {0x00, 0x40, 0x00, 0xC0, 0x11, 0x11}}));

    EXPECT_EQ(wav.frameCount(), 1U);
    EXPECT_EQ(wav.samples.size(), 2U);
}

TEST(WavReaderTest, Decode_ReadsADataChunkThatEndsExactlyAtTheStreamEnd)
{
    const auto bytes = build(Wav{.samples = {}});

    ASSERT_EQ(bytes.size(), 44U);

    const auto wav = decode(bytes);

    EXPECT_EQ(wav.format, (WaveFormat{.rate = 48000, .channels = 1}));
    EXPECT_EQ(wav.frameCount(), 0U);
    EXPECT_TRUE(wav.samples.empty());
}

TEST(WavReaderTest, Decode_ReadsChunksInEitherOrder)
{
    Bytes allBytes;
    allBytes.tag("RIFF");
    allBytes.u32(4 + 8 + 4 + 8 + 16);
    allBytes.tag("WAVE");

    allBytes.tag("data");
    allBytes.u32(4);
    allBytes.raw(0x00);
    allBytes.raw(0x40);
    allBytes.raw(0x00);
    allBytes.raw(0x00);

    allBytes.tag("fmt ");
    allBytes.u32(16);
    allBytes.u16(1);
    allBytes.u16(1);
    allBytes.u32(48000);
    allBytes.u32(0);
    allBytes.u16(0);
    allBytes.u16(16);

    EXPECT_EQ(decode(allBytes.str()).frameCount(), 2U);
}

TEST(WavReaderTest, Decode_SkipsAnUnknownChunk)
{
    Bytes allBytes;
    allBytes.tag("RIFF");
    allBytes.u32(4 + 8 + 3 + 1 + 8 + 16 + 8 + 2);
    allBytes.tag("WAVE");

    allBytes.tag("LIST");
    allBytes.u32(3);
    allBytes.raw('a');
    allBytes.raw('b');
    allBytes.raw('c');
    allBytes.raw(0);

    allBytes.tag("fmt ");
    allBytes.u32(16);
    allBytes.u16(1);
    allBytes.u16(1);
    allBytes.u32(48000);
    allBytes.u32(0);
    allBytes.u16(0);
    allBytes.u16(16);

    allBytes.tag("data");
    allBytes.u32(2);
    allBytes.raw(0x00);
    allBytes.raw(0x40);

    EXPECT_EQ(decode(allBytes.str()).frameCount(), 1U);
}

TEST(WavReaderTest, Decode_RefusesAnEmptyStream)
{
    EXPECT_THROW((void)decode(""), SoundError);
}

TEST(WavReaderTest, Decode_RefusesSomethingThatIsNotRiff)
{
    EXPECT_THROW((void)decode("not a wav file at all"), SoundError);
}

TEST(WavReaderTest, Decode_RefusesRiffThatIsNotWave)
{
    Bytes allBytes;
    allBytes.tag("RIFF");
    allBytes.u32(4);
    allBytes.tag("AVI ");

    EXPECT_THROW((void)decode(allBytes.str()), SoundError);
}

TEST(WavReaderTest, Decode_TakesABareRiffWaveHeaderAndThenMissesTheFormat)
{
    Bytes allBytes;
    allBytes.tag("RIFF");
    allBytes.u32(4);
    allBytes.tag("WAVE");

    ASSERT_EQ(allBytes.data.size(), 12U);

    try
    {
        (void)decode(allBytes.str());
        FAIL() << "a stream of twelve header bytes decoded";
    }
    catch (const SoundError &error)
    {
        EXPECT_NE(
            std::string(error.what()).find("holds no format chunk"),
            std::string::npos)
            << error.what();
    }
}

TEST(WavReaderTest, Decode_RefusesAChunkPastTheEnd)
{
    Bytes allBytes;
    allBytes.tag("RIFF");
    allBytes.u32(4 + 8);
    allBytes.tag("WAVE");
    allBytes.tag("data");
    allBytes.u32(9999);

    EXPECT_THROW((void)decode(allBytes.str()), SoundError);
}

TEST(WavReaderTest, Decode_RefusesAFileWithNoFormatChunk)
{
    EXPECT_THROW(
        (void)decode(build(Wav{.samples = {0, 0}, .withFmt = false})),
        SoundError);
}

TEST(WavReaderTest, Decode_RefusesAFileWithNoDataChunk)
{
    EXPECT_THROW(
        (void)decode(build(Wav{.samples = {}, .withData = false})),
        SoundError);
}

TEST(WavReaderTest, Decode_RefusesATooShortFormatChunk)
{
    Bytes allBytes;
    allBytes.tag("RIFF");
    allBytes.u32(4 + 8 + 4);
    allBytes.tag("WAVE");
    allBytes.tag("fmt ");
    allBytes.u32(4);
    allBytes.u32(0);

    EXPECT_THROW((void)decode(allBytes.str()), SoundError);
}

TEST(WavReaderTest, Decode_RefusesNoSubFormatWhenExtensible)
{
    EXPECT_THROW(
        (void)decode(build(
            Wav{.encoding = 0xFFFE, .samples = {0, 0}, .fmtSize = 18})),
        SoundError);
}

TEST(WavReaderTest, Decode_RefusesAnUnknownCompression)
{
    EXPECT_THROW(
        (void)decode(build(Wav{.encoding = 6, .samples = {0, 0}})),
        SoundError);
}

TEST(WavReaderTest, Decode_RefusesANonThirtyTwoBitFloat)
{
    EXPECT_THROW(
        (void)decode(build(
            Wav{.encoding = 3, .bits = 64, .samples = {0, 0}})),
        SoundError);
}

TEST(WavReaderTest, Decode_RefusesAnUnknownSampleWidth)
{
    EXPECT_THROW(
        (void)decode(build(Wav{.bits = 12, .samples = {0, 0}})),
        SoundError);
}

TEST(WavReaderTest, Decode_RefusesASampleRateOfZero)
{
    EXPECT_THROW(
        (void)decode(build(Wav{.rate = 0, .samples = {0, 0}})),
        SoundError);
}

TEST(WavReaderTest, Decode_RefusesTooFewOrTooManyChannels)
{
    EXPECT_THROW(
        (void)decode(build(Wav{.channels = 0, .samples = {0, 0}})),
        SoundError);

    EXPECT_THROW(
        (void)decode(build(Wav{.channels = 99, .samples = {0, 0}})),
        SoundError);
}
