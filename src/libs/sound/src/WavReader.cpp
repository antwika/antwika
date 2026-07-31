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
        // The two encodings this decodes, out of the many WAVE names.
        constexpr std::uint16_t kFormatPcm = 1;
        constexpr std::uint16_t kFormatFloat = 3;
        constexpr std::uint16_t kFormatExtensible = 0xFFFE;

        [[nodiscard]] std::vector<std::uint8_t> readAll(std::istream &in)
        {
            return std::vector<std::uint8_t>(
                std::istreambuf_iterator<char>(in),
                std::istreambuf_iterator<char>());
        }

        [[nodiscard]] std::uint32_t readU32(
            const std::vector<std::uint8_t> &bytes, std::size_t at)
        {
            return static_cast<std::uint32_t>(bytes[at])
                | (static_cast<std::uint32_t>(bytes[at + 1]) << 8U)
                | (static_cast<std::uint32_t>(bytes[at + 2]) << 16U)
                | (static_cast<std::uint32_t>(bytes[at + 3]) << 24U);
        }

        [[nodiscard]] std::uint16_t readU16(
            const std::vector<std::uint8_t> &bytes, std::size_t at)
        {
            return static_cast<std::uint16_t>(
                static_cast<std::uint16_t>(bytes[at])
                | static_cast<std::uint16_t>(bytes[at + 1] << 8U));
        }

        // Every caller has already established that four bytes are there.
        // The header check reads offsets 0 and 8 of at least twelve.
        // The chunk walk only enters its body with eight bytes in hand.
        // So a bounds test here would be one no call site could fail.
        [[nodiscard]] bool tagAt(
            const std::vector<std::uint8_t> &bytes,
            std::size_t at,
            const char *tag)
        {
            return std::memcmp(bytes.data() + at, tag, 4) == 0;
        }

        struct Fmt
        {
            std::uint16_t encoding = 0;
            std::uint16_t channels = 0;
            std::uint32_t rate = 0;
            std::uint16_t bits = 0;
        };

        // One sample as a float in roughly -1 to +1.
        // Whatever width it was stored at arrives here the same way.
        [[nodiscard]] float sampleAt(
            const std::vector<std::uint8_t> &bytes,
            std::size_t at,
            const Fmt &fmt)
        {
            if (fmt.encoding == kFormatFloat)
            {
                float value = 0.0F;
                std::memcpy(&value, bytes.data() + at, sizeof(value));
                return value;
            }

            if (fmt.bits == 8)
            {
                // Eight-bit WAV is unsigned, and every wider one is not.
                return (static_cast<float>(bytes[at]) - 128.0F) / 128.0F;
            }

            if (fmt.bits == 16)
            {
                const auto raw =
                    static_cast<std::int16_t>(readU16(bytes, at));
                return static_cast<float>(raw) / 32768.0F;
            }

            if (fmt.bits == 24)
            {
                auto raw = static_cast<std::int32_t>(
                    (static_cast<std::uint32_t>(bytes[at]) << 8U)
                    | (static_cast<std::uint32_t>(bytes[at + 1]) << 16U)
                    | (static_cast<std::uint32_t>(bytes[at + 2]) << 24U));
                return static_cast<float>(raw >> 8) / 8388608.0F;
            }

            const auto raw = static_cast<std::int32_t>(readU32(bytes, at));
            return static_cast<float>(raw) / 2147483648.0F;
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
    } // namespace

    Waveform WavReader::read(std::istream &in) const
    {
        const auto bytes = readAll(in);

        // Twelve bytes is the RIFF header alone.
        // Anything shorter cannot even say what it is.
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

        // Chunks in any order, each padded to an even length.
        std::size_t at = 12;

        while (at + 8 <= bytes.size())
        {
            const auto size = static_cast<std::size_t>(readU32(bytes, at + 4));
            const auto body = at + 8;

            if (body + size > bytes.size())
            {
                throw SoundError(
                    "antwika::sound: a WAV chunk runs past the end of the "
                    "stream");
            }

            if (tagAt(bytes, at, "fmt "))
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

                // An extensible header names a sub-format instead.
                // That GUID opens with the encoding other files state.
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
            else if (tagAt(bytes, at, "data"))
            {
                dataAt = body;
                dataSize = size;
                haveData = true;
            }

            at = body + size + (size % 2);
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

        // No ceiling is stated on how many samples a header may claim.
        // The chunk walk above already refused a size the stream lacks.
        // So the bytes are in hand.
        // What a corrupt header claims cannot exceed what was read.
        const auto width = static_cast<std::size_t>(fmt.bits) / 8;
        const auto count = dataSize / width;

        Waveform waveform;
        waveform.format = WaveFormat{
            .rate = fmt.rate,
            .channels = static_cast<ChannelCount>(fmt.channels)};

        // Trailing bytes short of a whole frame are dropped.
        // A file padded to a block boundary is ordinary.
        // Its audio is still perfectly good, so refusing it helps nobody.
        const auto frames = count / fmt.channels;
        waveform.samples.reserve(frames * fmt.channels);

        for (std::size_t index = 0; index < frames * fmt.channels; ++index)
        {
            waveform.samples.push_back(
                sampleAt(bytes, dataAt + index * width, fmt));
        }

        return waveform;

        // The excluded line is the local waveform's unwind destructor.
        // Nothing between its construction and the return throws.
    } // GCOVR_EXCL_LINE

} // namespace antwika::sound
