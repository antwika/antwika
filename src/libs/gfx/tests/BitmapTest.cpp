#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include "antwika/gfx/Bitmap.hpp"
#include "antwika/gfx/Size.hpp"

using antwika::gfx::Bitmap;
using antwika::gfx::kBytesPerPixel;
using antwika::gfx::Size;

namespace
{
    Bitmap bitmapOf(Size size, std::size_t byteCount)
    {
        return Bitmap{
            .size = size,
            .pixels = std::vector<std::uint8_t>(byteCount, 0)};
    }
} // namespace

TEST(BitmapTest, IsComplete_IsTrueWhenThePixelsMatchTheSize)
{
    EXPECT_TRUE(
        bitmapOf({.width = 2, .height = 2}, 4 * kBytesPerPixel)
            .isComplete());
}

TEST(BitmapTest, IsComplete_IsFalseWhenThereAreTooFewPixels)
{
    EXPECT_FALSE(
        bitmapOf({.width = 2, .height = 2}, 4 * kBytesPerPixel - 1)
            .isComplete());
}

TEST(BitmapTest, IsComplete_IsFalseWhenThereAreTooManyPixels)
{
    EXPECT_FALSE(
        bitmapOf({.width = 2, .height = 2}, 4 * kBytesPerPixel + 1)
            .isComplete());
}

TEST(BitmapTest, IsComplete_IsFalseWhenTheWidthIsZero)
{
    EXPECT_FALSE(bitmapOf({.width = 0, .height = 2}, 0).isComplete());
}

TEST(BitmapTest, IsComplete_IsFalseWhenTheHeightIsZero)
{
    EXPECT_FALSE(bitmapOf({.width = 2, .height = 0}, 0).isComplete());
}

TEST(BitmapTest, IsComplete_IsFalseForADefaultBitmap)
{
    EXPECT_FALSE(Bitmap{}.isComplete());
}

TEST(BitmapTest, Equality_ComparesTheSize)
{
    const auto wide = bitmapOf({.width = 4, .height = 1}, 0);
    const auto tall = bitmapOf({.width = 1, .height = 4}, 0);

    EXPECT_NE(wide, tall);
}

TEST(BitmapTest, Equality_ComparesEveryPixel)
{
    const Bitmap opaque{
        .size = {.width = 1, .height = 1},
        .pixels = {1, 2, 3, 255}};
    const Bitmap clear{
        .size = {.width = 1, .height = 1},
        .pixels = {1, 2, 3, 0}};

    EXPECT_EQ(opaque, opaque);
    EXPECT_NE(opaque, clear);
}
