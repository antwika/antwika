#include <gtest/gtest.h>

#include <cstdint>
#include <ios>
#include <sstream>
#include <string>
#include <vector>

#include "antwika/gfx/Bitmap.hpp"
#include "antwika/gfx/GfxError.hpp"
#include "antwika/image/PngReader.hpp"
#include "antwika/image/PngWriter.hpp"
#include "antwika/gfx/Size.hpp"

using antwika::gfx::Bitmap;
using antwika::gfx::GfxError;
using antwika::image::PngReader;
using antwika::image::PngWriter;
using antwika::gfx::Size;

namespace
{
    Bitmap getTwoByTwo()
    {
        return Bitmap{
            .size = {.width = 2, .height = 2},
            .pixels = std::vector<std::uint8_t>{
                255, 0,   0,   255, 0,  255, 0,   255,
                0,   0,   255, 0,   17, 34,  51,  128}};
    }

    std::string getWrittenBytes(const Bitmap &bitmap)
    {
        std::ostringstream outputStream;
        PngWriter{}.write(bitmap, outputStream);
        return outputStream.str();
    }
}

TEST(PngWriterTest, Write_ProducesBytesAPngReaderDecodesUnchanged)
{
    const auto original = getTwoByTwo();

    std::istringstream inputStream(getWrittenBytes(original));

    EXPECT_EQ(PngReader{}.read(inputStream), original);
}

TEST(PngWriterTest, Write_StartsWithThePngSignature)
{
    const auto bytes = getWrittenBytes(getTwoByTwo());

    ASSERT_GE(bytes.size(), 8U);
    EXPECT_EQ(
        bytes.substr(0, 8),
        std::string("\x89PNG\r\n\x1a\n", 8));
}

TEST(PngWriterTest, Write_RoundTripsEveryByteOfALargerImage)
{
    Bitmap originalBitmap{.size = {.width = 37, .height = 11}, .pixels = {}};
    originalBitmap.pixels.reserve(37U * 11U * 4U);

    for (std::uint32_t index = 0; index < 37U * 11U; ++index)
    {
        originalBitmap.pixels.push_back(static_cast<std::uint8_t>(index));
        originalBitmap.pixels.push_back(static_cast<std::uint8_t>(index * 3));
        originalBitmap.pixels.push_back(static_cast<std::uint8_t>(index * 7));
        originalBitmap.pixels.push_back(static_cast<std::uint8_t>(index % 256));
    }

    std::istringstream inputStream(getWrittenBytes(originalBitmap));

    EXPECT_EQ(PngReader{}.read(inputStream), originalBitmap);
}

TEST(PngWriterTest, Write_ThrowsOnABitmapWithTooFewPixels)
{
    const Bitmap truncatedBitmap{
        .size = {.width = 2, .height = 2},
        .pixels = std::vector<std::uint8_t>(8, 0)};

    std::ostringstream outputStream;

    EXPECT_THROW(PngWriter{}.write(truncatedBitmap, outputStream), GfxError);
}

TEST(PngWriterTest, Write_ThrowsOnABitmapWithNoPixelsAtAll)
{
    std::ostringstream outputStream;

    EXPECT_THROW(PngWriter{}.write(Bitmap{}, outputStream), GfxError);
}

TEST(PngWriterTest, Write_SaysWhatWasWrongWithTheBitmap)
{
    std::ostringstream outputStream;

    try
    {
        PngWriter{}.write(Bitmap{}, outputStream);
        FAIL() << "expected a GfxError";
    }
    catch (const GfxError &error)
    {
        EXPECT_NE(
            std::string(error.what()).find("does not hold the pixels"),
            std::string::npos);
    }
}

TEST(PngWriterTest, Write_ThrowsWhenTheStreamWillNotTakeTheBytes)
{
    std::ostringstream outputStream;
    outputStream.setstate(std::ios::badbit);

    EXPECT_THROW(PngWriter{}.write(getTwoByTwo(), outputStream), GfxError);
}

TEST(PngWriterTest, Write_SaysTheStreamWasWhatFailed)
{
    std::ostringstream outputStream;
    outputStream.setstate(std::ios::badbit);

    try
    {
        PngWriter{}.write(getTwoByTwo(), outputStream);
        FAIL() << "expected a GfxError";
    }
    catch (const GfxError &error)
    {
        EXPECT_NE(
            std::string(error.what()).find("would not take"),
            std::string::npos);
    }
}

TEST(PngWriterTest, Write_LeavesNothingBehindOnAWriterItRefused)
{
    std::ostringstream outputStream;

    EXPECT_THROW(PngWriter{}.write(Bitmap{}, outputStream), GfxError);
    EXPECT_TRUE(outputStream.str().empty());
}
