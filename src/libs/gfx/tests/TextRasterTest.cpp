#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

#include <antwika/gfx/Glyphs.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/TextRaster.hpp>

using antwika::gfx::forEachGlyphPixel;
using antwika::gfx::kGlyphAdvance;
using antwika::gfx::kGlyphHeight;
using antwika::gfx::kGlyphWidth;
using antwika::gfx::glyphRow;
using antwika::gfx::Point;
using antwika::gfx::Rect;
using antwika::gfx::Size;

namespace
{
    constexpr Point kOrigin{.x = 10, .y = 20};

    [[nodiscard]] std::vector<Rect> pixelsOf(
        Point origin, std::string_view text, std::uint32_t scale)
    {
        std::vector<Rect> pixels;

        forEachGlyphPixel(
            origin,
            text,
            scale,
            [&pixels](Rect pixel) { pixels.push_back(pixel); });

        return pixels;
    }

    // How many pixels one character lights, straight from the font.
    [[nodiscard]] std::size_t litPixels(char character)
    {
        std::size_t count = 0;

        for (std::uint32_t row = 0; row < kGlyphHeight; ++row)
        {
            const auto bits = glyphRow(character, row);

            for (std::uint32_t column = 0; column < kGlyphWidth;
                 ++column)
            {
                count += (bits >> column) & 1U;
            }
        }

        return count;
    }
} // namespace

TEST(TextRasterTest, ForEachGlyphPixel_VisitsNothingAtZeroScale)
{
    EXPECT_TRUE(pixelsOf(kOrigin, "A", 0).empty());
}

TEST(TextRasterTest, ForEachGlyphPixel_VisitsNothingForAnEmptyLine)
{
    EXPECT_TRUE(pixelsOf(kOrigin, "", 1).empty());
}

// A space is a cell the font lights nothing in.
// So is any character it has no glyph for.
TEST(TextRasterTest, ForEachGlyphPixel_VisitsNothingForABlankCell)
{
    EXPECT_TRUE(pixelsOf(kOrigin, " ", 1).empty());
}

// Every lit bit of the font is one visit, and no more.
TEST(TextRasterTest, ForEachGlyphPixel_VisitsEveryLitBitOnce)
{
    EXPECT_EQ(litPixels('A'), pixelsOf(kOrigin, "A", 1).size());
    EXPECT_EQ(
        litPixels('A') + litPixels('B'),
        pixelsOf(kOrigin, "AB", 1).size());
}

// The first lit pixel of a glyph is the one that pins the origin.
TEST(TextRasterTest, ForEachGlyphPixel_StartsAtTheCellsOwnCorner)
{
    const auto pixels = pixelsOf(kOrigin, "A", 1);

    ASSERT_FALSE(pixels.empty());

    // Row zero of 'A' is lit, so the first visit is on the top row.
    EXPECT_EQ(kOrigin.y, pixels.front().origin.y);
    EXPECT_EQ((Size{.width = 1, .height = 1}), pixels.front().size);

    for (const auto &pixel : pixels)
    {
        EXPECT_GE(pixel.origin.x, kOrigin.x);
        EXPECT_LT(
            pixel.origin.x,
            kOrigin.x + static_cast<std::int32_t>(kGlyphWidth));
        EXPECT_GE(pixel.origin.y, kOrigin.y);
        EXPECT_LT(
            pixel.origin.y,
            kOrigin.y + static_cast<std::int32_t>(kGlyphHeight));
    }
}

// One cell along per character, at the advance the metrics promise.
TEST(TextRasterTest, ForEachGlyphPixel_AdvancesOneCellPerCharacter)
{
    const auto alone = pixelsOf(kOrigin, "A", 1);
    const auto second = pixelsOf(
        Point{
            .x = kOrigin.x + static_cast<std::int32_t>(kGlyphAdvance),
            .y = kOrigin.y},
        "A",
        1);
    const auto pair = pixelsOf(kOrigin, " A", 1);

    ASSERT_EQ(second.size(), pair.size());

    // A leading space lights nothing, so the pair is the second cell.
    for (std::size_t index = 0; index < pair.size(); ++index)
    {
        EXPECT_EQ(second.at(index), pair.at(index));
    }

    EXPECT_EQ(alone.size(), pair.size());
}

// A glyph pixel is a square of the scale, placed at that many pixels.
TEST(TextRasterTest, ForEachGlyphPixel_ScalesBothTheStepAndTheSquare)
{
    const auto plain = pixelsOf(Point{.x = 0, .y = 0}, "A", 1);
    const auto tripled = pixelsOf(Point{.x = 0, .y = 0}, "A", 3);

    ASSERT_EQ(plain.size(), tripled.size());

    for (std::size_t index = 0; index < plain.size(); ++index)
    {
        EXPECT_EQ(plain.at(index).origin.x * 3, tripled.at(index).origin.x);
        EXPECT_EQ(plain.at(index).origin.y * 3, tripled.at(index).origin.y);
        EXPECT_EQ(3U, tripled.at(index).size.width);
        EXPECT_EQ(3U, tripled.at(index).size.height);
    }
}
