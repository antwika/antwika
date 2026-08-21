#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include "antwika/gfx/Bitmap.hpp"
#include "antwika/gfx/Color.hpp"
#include "antwika/gfx/Size.hpp"

using antwika::gfx::Bitmap;
using antwika::gfx::Color;
using antwika::gfx::colorAt;
using antwika::gfx::kBytesPerPixel;
using antwika::gfx::setColorAt;
using antwika::gfx::Size;

namespace
{
    Bitmap bitmapOf(Size size, std::size_t byteCount)
    {
        return Bitmap{
            .size = size,
            .pixels = std::vector<std::uint8_t>(byteCount, 0)};
    }
}

TEST(BitmapTest, IsValid_IsTrueWhenThePixelsMatchTheSize)
{
    EXPECT_TRUE(
        bitmapOf({.width = 2, .height = 2}, 4 * kBytesPerPixel)
            .isValid());
}

TEST(BitmapTest, IsValid_IsFalseWhenThereAreTooFewPixels)
{
    EXPECT_FALSE(
        bitmapOf({.width = 2, .height = 2}, 4 * kBytesPerPixel - 1)
            .isValid());
}

TEST(BitmapTest, IsValid_IsFalseWhenThereAreTooManyPixels)
{
    EXPECT_FALSE(
        bitmapOf({.width = 2, .height = 2}, 4 * kBytesPerPixel + 1)
            .isValid());
}

TEST(BitmapTest, IsValid_IsFalseWhenTheWidthIsZero)
{
    EXPECT_FALSE(bitmapOf({.width = 0, .height = 2}, 0).isValid());
}

TEST(BitmapTest, IsValid_IsFalseWhenTheHeightIsZero)
{
    EXPECT_FALSE(bitmapOf({.width = 2, .height = 0}, 0).isValid());
}

TEST(BitmapTest, IsValid_IsFalseForADefaultBitmap)
{
    EXPECT_FALSE(Bitmap{}.isValid());
}

TEST(BitmapTest, OperatorEquals_ComparesTheSize)
{
    const auto wideBitmap = bitmapOf({.width = 4, .height = 1}, 0);
    const auto tallBitmap = bitmapOf({.width = 1, .height = 4}, 0);

    EXPECT_NE(wideBitmap, tallBitmap);
}

TEST(BitmapTest, OperatorEquals_ComparesEveryPixel)
{
    const Bitmap opaqueBitmap{
        .size = {.width = 1, .height = 1},
        .pixels = {1, 2, 3, 255}};
    const Bitmap clearBitmap{
        .size = {.width = 1, .height = 1},
        .pixels = {1, 2, 3, 0}};

    const auto twin = opaqueBitmap;
    EXPECT_EQ(opaqueBitmap, twin);
    EXPECT_NE(opaqueBitmap, clearBitmap);
}

TEST(BitmapTest, ColorAt_ReadsThePixelAtAPlace)
{
    const Bitmap sheetBitmap{
        .size = {.width = 2, .height = 1},
        .pixels = {1, 2, 3, 4, 5, 6, 7, 8}};

    EXPECT_EQ(
        colorAt(sheetBitmap, 1, 0),
        (Color{.red = 5, .green = 6, .blue = 7, .alpha = 8}));
}

TEST(BitmapTest, ColorAt_IsEmptyOutsideTheBitmap)
{
    const Bitmap sheetBitmap{
        .size = {.width = 2, .height = 2},
        .pixels = std::vector<std::uint8_t>(
            4 * kBytesPerPixel, 0)};

    EXPECT_FALSE(colorAt(sheetBitmap, -1, 0).has_value());
    EXPECT_FALSE(colorAt(sheetBitmap, 0, -1).has_value());
    EXPECT_FALSE(colorAt(sheetBitmap, 2, 0).has_value());
    EXPECT_FALSE(colorAt(sheetBitmap, 0, 2).has_value());
}

TEST(BitmapTest, ColorAt_IsEmptyBeyondThePixelStore)
{
    const Bitmap sheetBitmap{
        .size = {.width = 2, .height = 2},
        .pixels = std::vector<std::uint8_t>(kBytesPerPixel, 0)};

    EXPECT_TRUE(colorAt(sheetBitmap, 0, 0).has_value());
    EXPECT_FALSE(colorAt(sheetBitmap, 1, 1).has_value());
}

TEST(BitmapTest, SetColorAt_WritesThePixelAtAPlace)
{
    Bitmap sheetBitmap{
        .size = {.width = 2, .height = 1},
        .pixels = std::vector<std::uint8_t>(
            2 * kBytesPerPixel, 0)};

    setColorAt(
        sheetBitmap,
        1,
        0,
        Color{.red = 9, .green = 8, .blue = 7, .alpha = 6});

    EXPECT_EQ(
        sheetBitmap.pixels,
        (std::vector<std::uint8_t>{0, 0, 0, 0, 9, 8, 7, 6}));
}

TEST(BitmapTest, SetColorAt_DoesNothingOutsideTheBitmap)
{
    Bitmap sheetBitmap{
        .size = {.width = 1, .height = 1},
        .pixels = std::vector<std::uint8_t>(kBytesPerPixel, 0)};

    const auto was = sheetBitmap;

    setColorAt(sheetBitmap, -1, 0, Color{.red = 9});
    setColorAt(sheetBitmap, 0, -1, Color{.red = 9});
    setColorAt(sheetBitmap, 1, 0, Color{.red = 9});
    setColorAt(sheetBitmap, 0, 1, Color{.red = 9});

    EXPECT_EQ(sheetBitmap, was);
}
