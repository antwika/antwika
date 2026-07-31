#include <gtest/gtest.h>

#include <cstdint>
#include <ios>
#include <sstream>
#include <string>
#include <vector>

#include "antwika/gfx/Bitmap.hpp"
#include "antwika/gfx/GfxError.hpp"
#include "antwika/gfx/PngReader.hpp"
#include "antwika/gfx/PngWriter.hpp"
#include "antwika/gfx/Size.hpp"

using antwika::gfx::Bitmap;
using antwika::gfx::GfxError;
using antwika::gfx::PngReader;
using antwika::gfx::PngWriter;
using antwika::gfx::Size;

namespace
{
    // Every channel value that matters is in here.
    // Fully opaque, fully clear, and a partial alpha over a colour.
    // A writer that premultiplied could not round-trip the last one.
    // Nor could one that dropped the alpha channel.
    Bitmap twoByTwo()
    {
        return Bitmap{
            .size = {.width = 2, .height = 2},
            .pixels = std::vector<std::uint8_t>{
                255, 0,   0,   255, 0,  255, 0,   255,
                0,   0,   255, 0,   17, 34,  51,  128}};
    }

    std::string writtenBytes(const Bitmap &bitmap)
    {
        std::ostringstream out;
        PngWriter{}.write(bitmap, out);
        return out.str();
    }
} // namespace

TEST(PngWriterTest, Write_ProducesBytesAPngReaderDecodesUnchanged)
{
    const auto original = twoByTwo();

    std::istringstream in(writtenBytes(original));

    EXPECT_EQ(PngReader{}.read(in), original);
}

TEST(PngWriterTest, Write_StartsWithThePngSignature)
{
    const auto bytes = writtenBytes(twoByTwo());

    ASSERT_GE(bytes.size(), 8U);
    EXPECT_EQ(
        bytes.substr(0, 8),
        std::string("\x89PNG\r\n\x1a\n", 8));
}

TEST(PngWriterTest, Write_RoundTripsEveryByteOfALargerImage)
{
    // Wide enough to span several filtered scanlines rather than one.
    // It is also a size no power-of-two assumption would flatter.
    Bitmap original{.size = {.width = 37, .height = 11}, .pixels = {}};
    original.pixels.reserve(37U * 11U * 4U);

    for (std::uint32_t index = 0; index < 37U * 11U; ++index)
    {
        original.pixels.push_back(static_cast<std::uint8_t>(index));
        original.pixels.push_back(static_cast<std::uint8_t>(index * 3));
        original.pixels.push_back(static_cast<std::uint8_t>(index * 7));
        original.pixels.push_back(static_cast<std::uint8_t>(index % 256));
    }

    std::istringstream in(writtenBytes(original));

    EXPECT_EQ(PngReader{}.read(in), original);
}

TEST(PngWriterTest, Write_ThrowsOnABitmapWithTooFewPixels)
{
    const Bitmap truncated{
        .size = {.width = 2, .height = 2},
        .pixels = std::vector<std::uint8_t>(8, 0)};

    std::ostringstream out;

    EXPECT_THROW(PngWriter{}.write(truncated, out), GfxError);
}

TEST(PngWriterTest, Write_ThrowsOnABitmapWithNoPixelsAtAll)
{
    std::ostringstream out;

    EXPECT_THROW(PngWriter{}.write(Bitmap{}, out), GfxError);
}

TEST(PngWriterTest, Write_SaysWhatWasWrongWithTheBitmap)
{
    std::ostringstream out;

    try
    {
        PngWriter{}.write(Bitmap{}, out);
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
    std::ostringstream out;
    out.setstate(std::ios::badbit);

    EXPECT_THROW(PngWriter{}.write(twoByTwo(), out), GfxError);
}

TEST(PngWriterTest, Write_SaysTheStreamWasWhatFailed)
{
    std::ostringstream out;
    out.setstate(std::ios::badbit);

    try
    {
        PngWriter{}.write(twoByTwo(), out);
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
    std::ostringstream out;

    EXPECT_THROW(PngWriter{}.write(Bitmap{}, out), GfxError);
    EXPECT_TRUE(out.str().empty());
}
