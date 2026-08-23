#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ios>
#include <string>
#include <system_error>
#include <unistd.h>

#include <antwika/app/WavFile.hpp>
#include <antwika/sound/SoundError.hpp>
#include <antwika/testing/ScratchPath.hpp>
#include <antwika/testing/ScratchFile.hpp>

using antwika::app::getReadWavFile;
using antwika::sound::SoundError;

namespace
{
    constexpr std::array<std::uint8_t, 48> kTwoFrameWav{
        'R',  'I',  'F',  'F',  0x28, 0x00, 0x00, 0x00, 'W',  'A',
        'V',  'E',  'f',  'm',  't',  ' ',  0x10, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x01, 0x00, 0x80, 0xbb, 0x00, 0x00, 0x00, 0x77,
        0x01, 0x00, 0x02, 0x00, 0x10, 0x00, 'd',  'a',  't',  'a',
        0x04, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00};
}

TEST(WavFileTest, ReadWavFile_ReadsAWavOffDisk)
{
    const antwika::testing::ScratchFile file("antwika-app-two-frames.wav");
    {
        std::ofstream outputStream(file.getString(), std::ios::binary);
        outputStream.write(
            reinterpret_cast<const char *>(kTwoFrameWav.data()),
            static_cast<std::streamsize>(kTwoFrameWav.size()));
    }

    const auto wave = getReadWavFile(file.getString(), "antwika_test");

    EXPECT_EQ(wave.format.rate, 48000U);
    EXPECT_EQ(wave.format.channels, 1U);
    EXPECT_EQ(wave.getFrameCount(), 2U);
}

TEST(WavFileTest, ReadWavFile_NamesTheMissingFileAndCaller)
{
    const auto missing =
        (std::filesystem::temp_directory_path() / "antwika-no-such.wav")
            .string();

    try
    {
        static_cast<void>(getReadWavFile(missing, "antwika_test"));
        FAIL() << "expected a SoundError";
    }
    catch (const SoundError &error)
    {
        const std::string message = error.what();
        EXPECT_NE(message.find("antwika_test"), std::string::npos);
        EXPECT_NE(message.find(missing), std::string::npos);
    }
}
