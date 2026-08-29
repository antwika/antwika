#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/render/Checkerboard.hpp"

using antwika::render::getCheckerboardBitmap;
using antwika::gfx::Bitmap;
using antwika::gfx::Color;
using antwika::gfx::Size;

namespace
{
    constexpr Size kBoardSize{.width = 4, .height = 4};

    [[nodiscard]] Color getPixel(
        const Bitmap &bitmap,
        const std::uint32_t row,
        const std::uint32_t column)
    {
        const auto pixelIndex =
            (static_cast<std::size_t>(row) * bitmap.size.width + column)
            * antwika::gfx::kBytesPerPixel;

        return Color{
            .red = bitmap.pixels.at(pixelIndex),
            .green = bitmap.pixels.at(pixelIndex + 1),
            .blue = bitmap.pixels.at(pixelIndex + 2),
            .alpha = bitmap.pixels.at(pixelIndex + 3)};
    }
}

TEST(CheckerboardTest, GetCheckerboardBitmap_FillsEveryPixelOfTheBoard)
{
    const auto bitmap = getCheckerboardBitmap(kBoardSize);

    EXPECT_EQ(bitmap.size, kBoardSize);
    EXPECT_EQ(
        bitmap.pixels.size(),
        static_cast<std::size_t>(kBoardSize.width)
            * kBoardSize.height * antwika::gfx::kBytesPerPixel);
}

TEST(CheckerboardTest, GetCheckerboardBitmap_AlternatesNeighboursAtCheckOne)
{
    const auto bitmap = getCheckerboardBitmap(kBoardSize, 1);

    EXPECT_NE(getPixel(bitmap, 0, 0), getPixel(bitmap, 0, 1));
    EXPECT_NE(getPixel(bitmap, 0, 0), getPixel(bitmap, 1, 0));
    EXPECT_EQ(getPixel(bitmap, 0, 0), getPixel(bitmap, 1, 1));
}

TEST(CheckerboardTest, GetCheckerboardBitmap_HoldsEachCheckOfTwoTogether)
{
    const auto bitmap = getCheckerboardBitmap(kBoardSize, 2);

    EXPECT_EQ(getPixel(bitmap, 0, 0), getPixel(bitmap, 0, 1));
    EXPECT_EQ(getPixel(bitmap, 0, 0), getPixel(bitmap, 1, 1));
    EXPECT_NE(getPixel(bitmap, 0, 0), getPixel(bitmap, 0, 2));
    EXPECT_NE(getPixel(bitmap, 0, 0), getPixel(bitmap, 2, 0));
}

TEST(CheckerboardTest, GetCheckerboardBitmap_TreatsACheckOfZeroAsOne)
{
    const auto degenerate = getCheckerboardBitmap(kBoardSize, 0);
    const auto smallest = getCheckerboardBitmap(kBoardSize, 1);

    EXPECT_EQ(degenerate.pixels, smallest.pixels);
}
