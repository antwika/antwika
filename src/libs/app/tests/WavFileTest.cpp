#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ios>
#include <string>
#include <system_error>

#include <gtest/gtest.h>

#include <antwika/app/WavFile.hpp>
#include <antwika/sound/SoundError.hpp>

using antwika::app::readWavFile;
using antwika::sound::SoundError;

namespace
{
    // A two-frame 16-bit mono WAV at 48 kHz.
    // Written out by the test rather than checked in beside it.
    // Exactly as PngFileTest's one-pixel image is.
    // This module's tests then need no asset of their own.
    constexpr std::array<std::uint8_t, 48> kTwoFrameWav{
        'R',  'I',  'F',  'F',  0x28, 0x00, 0x00, 0x00, 'W',  'A',
        'V',  'E',  'f',  'm',  't',  ' ',  0x10, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x01, 0x00, 0x80, 0xbb, 0x00, 0x00, 0x00, 0x77,
        0x01, 0x00, 0x02, 0x00, 0x10, 0x00, 'd',  'a',  't',  'a',
        0x04, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00};

    /**
     * @brief A file in the temporary directory, removed when it goes.
     */
    class TempFile final
    {
    public:
        explicit TempFile(const std::string &name)
            : path(std::filesystem::temp_directory_path() / name)
        {
        }

        TempFile(const TempFile &) = delete;
        TempFile(TempFile &&) = delete;

        TempFile &operator=(const TempFile &) = delete;
        TempFile &operator=(TempFile &&) = delete;

        ~TempFile()
        {
            std::error_code ignored;
            std::filesystem::remove(path, ignored);
        }

        [[nodiscard]] std::string name() const
        {
            return path.string();
        }

    private:
        std::filesystem::path path;
    };
} // namespace

TEST(WavFileTest, ReadsAWavOffDisk)
{
    const TempFile file("antwika-app-two-frames.wav");
    {
        std::ofstream out(file.name(), std::ios::binary);
        out.write(
            reinterpret_cast<const char *>(kTwoFrameWav.data()),
            static_cast<std::streamsize>(kTwoFrameWav.size()));
    }

    const auto wave = readWavFile(file.name(), "antwika_test");

    EXPECT_EQ(wave.format.rate, 48000U);
    EXPECT_EQ(wave.format.channels, 1U);
    EXPECT_EQ(wave.frameCount(), 2U);
}

TEST(WavFileTest, SaysWhichFileIsMissingAndWhoWasLookingForIt)
{
    const auto missing =
        (std::filesystem::temp_directory_path() / "antwika-no-such.wav")
            .string();

    try
    {
        static_cast<void>(readWavFile(missing, "antwika_test"));
        FAIL() << "expected a SoundError";
    }
    catch (const SoundError &error)
    {
        const std::string message = error.what();
        EXPECT_NE(message.find("antwika_test"), std::string::npos);
        EXPECT_NE(message.find(missing), std::string::npos);
    }
}
