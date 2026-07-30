#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ios>
#include <string>
#include <system_error>

#include <gtest/gtest.h>

#include <antwika/app/PngFile.hpp>
#include <antwika/gfx/GfxError.hpp>

using antwika::app::readPngFile;
using antwika::gfx::GfxError;

namespace
{
    // A 1x1 RGBA PNG, which is the smallest thing PngReader accepts.
    // Written out by the test rather than checked in beside it.
    // This module's tests then need no asset of their own.
    constexpr std::array<std::uint8_t, 70> kOnePixelPng{
        0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00,
        0x00, 0x0d, 0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x01, 0x08, 0x06, 0x00, 0x00, 0x00, 0x1f,
        0x15, 0xc4, 0x89, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x44, 0x41,
        0x54, 0x78, 0xda, 0x63, 0xfc, 0xcf, 0xc0, 0x50, 0x0f, 0x00,
        0x04, 0x85, 0x01, 0x80, 0x84, 0xa9, 0x8c, 0x21, 0x00, 0x00,
        0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82};

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

TEST(PngFileTest, ReadsAPngOffDisk)
{
    const TempFile file("antwika-app-one-pixel.png");
    {
        std::ofstream out(file.name(), std::ios::binary);
        out.write(
            reinterpret_cast<const char *>(kOnePixelPng.data()),
            static_cast<std::streamsize>(kOnePixelPng.size()));
    }

    const auto bitmap = readPngFile(file.name(), "antwika_test");

    EXPECT_EQ(bitmap.size.width, 1U);
    EXPECT_EQ(bitmap.size.height, 1U);
}

TEST(PngFileTest, SaysWhichFileIsMissingAndWhoWasLookingForIt)
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
