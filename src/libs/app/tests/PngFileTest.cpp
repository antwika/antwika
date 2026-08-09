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

#include <antwika/app/PngFile.hpp>
#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/GfxError.hpp>
#include <antwika/testing/ScratchPath.hpp>

using antwika::app::readPngFile;
using antwika::app::writePngFile;
using antwika::gfx::GfxError;

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
        std::ofstream out(file.string(), std::ios::binary);
        out.write(
            reinterpret_cast<const char *>(kOnePixelPng.data()),
            static_cast<std::streamsize>(kOnePixelPng.size()));
    }

    const auto bitmap = readPngFile(file.string(), "antwika_test");

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
        static_cast<void>(readPngFile(missing, "antwika_test"));
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
        antwika::testing::scratchPath("png-file-written.png").string();

    const antwika::gfx::Bitmap page{
        .size = {.width = 2, .height = 1},
        .pixels = std::vector<std::uint8_t>{
            255, 0, 0, 255, 0, 255, 0, 255}};

    writePngFile(page, path, "test");

    EXPECT_EQ(readPngFile(path, "test"), page);

    std::error_code failed;
    std::filesystem::remove(path, failed);
}

TEST(PngFileTest, WritePngFile_SaysWhichFileItCouldNotWrite)
{
    const antwika::gfx::Bitmap page{
        .size = {.width = 1, .height = 1},
        .pixels = std::vector<std::uint8_t>{0, 0, 0, 255}};

    EXPECT_THROW(
        writePngFile(page, "/no/such/directory/frame.png", "test"),
        GfxError);
}
