#include "antwika/sound/WavReader.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <istream>
#include <string>
#include <vector>

#include "antwika/sound/SoundError.hpp"

namespace antwika::sound
{

    namespace
    {
        constexpr std::uint16_t kFormatPcm = 1;
        constexpr std::uint16_t kFormatFloat = 3;
        constexpr std::uint16_t kFormatExtensible = 0xFFFE;

        [[nodiscard]] std::vector<std::uint8_t> readAll(
            std::istream &inputStream)
        {
            return std::vector<std::uint8_t>(
                std::istreambuf_iterator<char>(inputStream),
                std::istreambuf_iterator<char>());
        }

        [[nodiscard]] std::uint32_t readU32(
            const std::vector<std::uint8_t> &bytes, std::size_t offset)
        {
            return static_cast<std::uint32_t>(bytes[offset])
                | (static_cast<std::uint32_t>(bytes[offset + 1]) << 8U)
                | (static_cast<std::uint32_t>(bytes[offset + 2]) << 16U)
                | (static_cast<std::uint32_t>(bytes[offset + 3]) << 24U);
        }

        [[nodiscard]] std::uint16_t readU16(
            const std::vector<std::uint8_t> &bytes, std::size_t offset)
        {
            return static_cast<std::uint16_t>(
                static_cast<std::uint16_t>(bytes[offset])
                | static_cast<std::uint16_t>(bytes[offset + 1] << 8U));
        }

        [[nodiscard]] bool tagAt(
            const std::vector<std::uint8_t> &bytes,
            std::size_t offset,
            const char *tag)
        {
            return std::memcmp(bytes.data() + offset, tag, 4) == 0;
        }

        struct Fmt final
        {
            std::uint16_t encoding = 0;
            std::uint16_t channels = 0;
            std::uint32_t rate = 0;
            std::uint16_t bits = 0;
        };

        [[nodiscard]] float sampleAt(
            const std::vector<std::uint8_t> &bytes,
            std::size_t offset,
            const Fmt &fmt)
        {
            if (fmt.encoding == kFormatFloat)
            {
                float value = 0.0F;
                std::memcpy(&value, bytes.data() + offset, sizeof(value));
                return value;
            }

            if (fmt.bits == 8)
            {
                return (static_cast<float>(bytes[offset]) - 128.0F) / 128.0F;
            }

            if (fmt.bits == 16)
            {
                const auto sampleValue =
                    static_cast<std::int16_t>(readU16(bytes, offset));
                return static_cast<float>(sampleValue) / 32768.0F;
            }

            if (fmt.bits == 24)
            {
                auto sampleValue = static_cast<std::int32_t>(
                    (static_cast<std::uint32_t>(bytes[offset]) << 8U)
                    | (static_cast<std::uint32_t>(bytes[offset + 1]) << 16U)
                    | (static_cast<std::uint32_t>(bytes[offset + 2]) << 24U));
                return static_cast<float>(sampleValue >> 8) / 8388608.0F;
            }

            const auto sampleValue =
                static_cast<std::int32_t>(readU32(bytes, offset));
            return static_cast<float>(sampleValue) / 2147483648.0F;
        }

        void requireDecodable(const Fmt &fmt)
        {
            if (fmt.encoding != kFormatPcm && fmt.encoding != kFormatFloat)
            {
                throw SoundError(
                    "antwika::sound: a WAV names encoding "
                    + std::to_string(fmt.encoding)
                    + ", and this decodes only PCM and float");
            }

            if (fmt.encoding == kFormatFloat && fmt.bits != 32)
            {
                throw SoundError(
                    "antwika::sound: a float WAV must be 32 bits, not "
                    + std::to_string(fmt.bits));
            }

            if (fmt.encoding == kFormatPcm && fmt.bits != 8 && fmt.bits != 16
                && fmt.bits != 24 && fmt.bits != 32)
            {
                throw SoundError(
                    "antwika::sound: a WAV holds "
                    + std::to_string(fmt.bits)
                    + "-bit samples, which this does not decode");
            }

            if (fmt.rate == 0)
            {
                throw SoundError(
                    "antwika::sound: a WAV names a sample rate of zero");
            }

            if (fmt.channels == 0 || fmt.channels > kMaxChannels)
            {
                throw SoundError(
                    "antwika::sound: a WAV names "
                    + std::to_string(fmt.channels)
                    + " channels, and this holds at most "
                    + std::to_string(kMaxChannels));
            }
        }
    }

    Waveform WavReader::read(std::istream &inputStream) const
    {
        const auto bytes = readAll(inputStream);

        if (bytes.size() < 12 || !tagAt(bytes, 0, "RIFF")
            || !tagAt(bytes, 8, "WAVE"))
        {
            throw SoundError(
                "antwika::sound: a stream does not begin with a RIFF WAVE "
                "header");
        }

        Fmt fmt;
        bool haveFmt = false;
        std::size_t dataAt = 0;
        std::size_t dataSize = 0;
        bool haveData = false;

        std::size_t offset = 12;

        while (offset + 8 <= bytes.size())
        {
            const auto size =
            static_cast<std::size_t>(readU32(bytes, offset + 4));
            const auto body = offset + 8;

            if (body + size > bytes.size())
            {
                throw SoundError(
                    "antwika::sound: a WAV chunk runs past the end of the "
                    "stream");
            }

            if (tagAt(bytes, offset, "fmt "))
            {
                if (size < 16)
                {
                    throw SoundError(
                        "antwika::sound: a WAV format chunk is too short "
                        "to describe anything");
                }

                fmt.encoding = readU16(bytes, body);
                fmt.channels = readU16(bytes, body + 2);
                fmt.rate = readU32(bytes, body + 4);
                fmt.bits = readU16(bytes, body + 14);

                if (fmt.encoding == kFormatExtensible)
                {
                    if (size < 26)
                    {
                        throw SoundError(
                            "antwika::sound: an extensible WAV format "
                            "chunk names no sub-format");
                    }

                    fmt.encoding = readU16(bytes, body + 24);
                }

                haveFmt = true;
            }
            else if (tagAt(bytes, offset, "data"))
            {
                dataAt = body;
                dataSize = size;
                haveData = true;
            }

            offset = body + size + (size % 2);
        }

        if (!haveFmt)
        {
            throw SoundError(
                "antwika::sound: a WAV holds no format chunk");
        }

        if (!haveData)
        {
            throw SoundError("antwika::sound: a WAV holds no data chunk");
        }

        requireDecodable(fmt);

        const auto width = static_cast<std::size_t>(fmt.bits) / 8;
        const auto count = dataSize / width;

        Waveform waveform;
        waveform.format = WaveFormat{
            .rate = fmt.rate,
            .channels = static_cast<ChannelCount>(fmt.channels)};

        const auto frames = count / fmt.channels;
        waveform.samples.reserve(frames * fmt.channels);

        for (std::size_t index = 0; index < frames * fmt.channels; ++index)
        {
            waveform.samples.push_back(
                sampleAt(bytes, dataAt + index * width, fmt));
        }

        return waveform;

    } // GCOVR_EXCL_LINE

}
