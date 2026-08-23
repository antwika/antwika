#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ios>
#include <string>
#include <vector>
#include <system_error>
#include <unistd.h>

#include <antwika/io/SafeWrite.hpp>
#include <antwika/testing/ScratchFile.hpp>
#include <antwika/testing/ScratchPath.hpp>

#include "antwika/gfx/Bitmap.hpp"
#include "antwika/gfx/GfxError.hpp"
#include "antwika/image/PngFile.hpp"

using antwika::gfx::GfxError;
using antwika::image::getReadPngFile;
using antwika::image::writePngFile;

namespace
{
    constexpr std::array<std::uint8_t, 70> kOnePixelPng{
        0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00,
        0x00, 0x0d, 0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x01, 0x08, 0x06, 0x00, 0x00, 0x00, 0x1f,
        0x15, 0xc4, 0x89, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x44, 0x41,
        0x54, 0x78, 0xda, 0x63, 0xfc, 0xcf, 0xc0, 0x50, 0x0f, 0x00,
        0x04, 0x85, 0x01, 0x80, 0x84, 0xa9, 0x8c, 0x21, 0x00, 0x00,
        0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82};
}

TEST(PngFileTest, ReadPngFile_ReadsAPngOffDisk)
{
    const antwika::testing::ScratchFile file("antwika-app-one-pixel.png");
    {
        std::ofstream outputStream(file.getString(), std::ios::binary);
        outputStream.write(
            reinterpret_cast<const char *>(kOnePixelPng.data()),
            static_cast<std::streamsize>(kOnePixelPng.size()));
    }

    const auto bitmap = getReadPngFile(file.getString(), "antwika_test");

    EXPECT_EQ(bitmap.size.width, 1U);
    EXPECT_EQ(bitmap.size.height, 1U);
}

TEST(PngFileTest, ReadPngFile_NamesTheMissingFileAndCaller)
{
    const auto missing =
        (std::filesystem::temp_directory_path() / "antwika-no-such.png")
            .string();

    try
    {
        static_cast<void>(getReadPngFile(missing, "antwika_test"));
        FAIL() << "expected a GfxError";
    }
    catch (const GfxError &error)
    {
        const std::string message = error.what();
        EXPECT_NE(message.find("antwika_test"), std::string::npos);
        EXPECT_NE(message.find(missing), std::string::npos);
    }
}

TEST(PngFileTest, WritePngFile_WritesWhatReadPngFileReadsBack)
{
    const auto path =
        antwika::testing::getScratchPath("png-file-written.png").string();

    const antwika::gfx::Bitmap pageBitmap{
        .size = {.width = 2, .height = 1},
        .pixels = std::vector<std::uint8_t>{
            255, 0, 0, 255, 0, 255, 0, 255}};

    writePngFile(pageBitmap, path, "test");

    EXPECT_EQ(getReadPngFile(path, "test"), pageBitmap);

    std::error_code errorCode;
    std::filesystem::remove(path, errorCode);
}

TEST(PngFileTest, WritePngFile_SaysWhichFileItCouldNotWrite)
{
    const antwika::gfx::Bitmap pageBitmap{
        .size = {.width = 1, .height = 1},
        .pixels = std::vector<std::uint8_t>{0, 0, 0, 255}};

    EXPECT_THROW(
        writePngFile(pageBitmap, "/no/such/directory/frame.png", "test"),
        GfxError);
}

TEST(PngFileTest, WritePngFile_KeepsTheArtworkItWritesOver)
{
    const auto path =
        antwika::testing::getScratchPath("png-file-kept.png").string();
    const auto backupPath = antwika::io::backupPathFor(path);
    const auto writingPath = antwika::io::writingPathFor(path);

    const antwika::gfx::Bitmap drawnFirstBitmap{
        .size = {.width = 2, .height = 1},
        .pixels = std::vector<std::uint8_t>{
            255, 0, 0, 255, 0, 255, 0, 255}};
    const antwika::gfx::Bitmap drawnOverBitmap{
        .size = {.width = 2, .height = 1},
        .pixels = std::vector<std::uint8_t>{
            0, 0, 255, 255, 255, 255, 0, 255}};

    std::error_code errorCode;

    std::filesystem::remove(path, errorCode);
    std::filesystem::remove(backupPath, errorCode);

    writePngFile(drawnFirstBitmap, path, "test");

    EXPECT_FALSE(std::filesystem::exists(backupPath));

    writePngFile(drawnOverBitmap, path, "test");

    ASSERT_TRUE(std::filesystem::exists(backupPath));
    EXPECT_EQ(getReadPngFile(path, "test"), drawnOverBitmap);
    EXPECT_EQ(getReadPngFile(backupPath, "test"), drawnFirstBitmap);
    EXPECT_FALSE(std::filesystem::exists(writingPath));

    std::filesystem::remove(path, errorCode);
    std::filesystem::remove(backupPath, errorCode);
}

TEST(PngFileTest, WritePngFile_LeavesTheArtworkAloneWhenItCannotWrite)
{
    const auto path =
        antwika::testing::getScratchPath("png-file-safe.png").string();

    const antwika::gfx::Bitmap drawnBitmap{
        .size = {.width = 1, .height = 1},
        .pixels = std::vector<std::uint8_t>{9, 9, 9, 255}};

    std::error_code errorCode;

    std::filesystem::remove(path, errorCode);
    writePngFile(drawnBitmap, path, "test");

    const antwika::gfx::Bitmap brokenBitmap{
        .size = {.width = 4, .height = 4},
        .pixels = std::vector<std::uint8_t>{0, 0, 0, 255}};

    EXPECT_THROW(writePngFile(brokenBitmap, path, "test"), GfxError);
    EXPECT_EQ(getReadPngFile(path, "test"), drawnBitmap);

    std::filesystem::remove(path, errorCode);
    std::filesystem::remove(
        antwika::io::writingPathFor(path), errorCode);
}
