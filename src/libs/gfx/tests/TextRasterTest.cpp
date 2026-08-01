#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <set>
#include <string_view>
#include <utility>
#include <vector>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/GlyphCells.hpp>
#include <antwika/gfx/Glyphs.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/TextLayout.hpp>
#include <antwika/gfx/TextRaster.hpp>

using antwika::gfx::Color;
using antwika::gfx::forEachGlyphPixel;
using antwika::gfx::glyphCells;
using antwika::gfx::glyphPixelColor;
using antwika::gfx::kGlyphAdvance;
using antwika::gfx::kGlyphLineHeight;
using antwika::gfx::Point;
using antwika::gfx::Rect;
using antwika::gfx::Size;
using antwika::gfx::textSize;

namespace
{
    constexpr Point kOrigin{.x = 10, .y = 20};
    constexpr Color kInk{
        .red = 200, .green = 100, .blue = 50, .alpha = 255};

    struct Visited
    {
        Rect pixel;
        Color color;
    };

    [[nodiscard]] std::vector<Visited> pixelsOf(
        Point origin,
        std::string_view text,
        std::uint32_t scale,
        Color color = kInk)
    {
        std::vector<Visited> pixels;

        forEachGlyphPixel(
            origin,
            text,
            scale,
            color,
            [&pixels](Rect pixel, Color pixelColor) {
                pixels.push_back(Visited{pixel, pixelColor});
            });

        return pixels;
    }

    // How many pixels one character inks, straight from the cells.
    [[nodiscard]] std::size_t inkedPixels(
        char character, std::uint32_t scale)
    {
        const auto &cells = glyphCells(scale);
        std::size_t count = 0;

        for (std::uint32_t row = 0; row < cells.cellSize().height; ++row)
        {
            for (std::uint32_t column = 0;
                 column < cells.cellSize().width;
                 ++column)
            {
                count += cells.coverageAt(character, column, row) != 0
                    ? 1U
                    : 0U;
            }
        }

        return count;
    }
} // namespace

TEST(TextRasterTest, GlyphPixelColor_ScalesTheAlphaByTheCoverage)
{
    EXPECT_EQ(kInk, glyphPixelColor(kInk, 255));
    EXPECT_EQ(
        (Color{
            .red = kInk.red,
            .green = kInk.green,
            .blue = kInk.blue,
            .alpha = 128}),
        glyphPixelColor(kInk, 128));
    EXPECT_EQ(0, glyphPixelColor(kInk, 0).alpha);
}

// Text drawn in a translucent colour stays translucent.
TEST(TextRasterTest, GlyphPixelColor_MultipliesRatherThanReplaces)
{
    constexpr Color faint{.red = 255, .alpha = 128};

    EXPECT_EQ(128, glyphPixelColor(faint, 255).alpha);
    EXPECT_EQ(64, glyphPixelColor(faint, 129).alpha);
}

TEST(TextRasterTest, ForEachGlyphPixel_VisitsNothingAtZeroScale)
{
    EXPECT_TRUE(pixelsOf(kOrigin, "A", 0).empty());
}

TEST(TextRasterTest, ForEachGlyphPixel_VisitsNothingForAnEmptyLine)
{
    EXPECT_TRUE(pixelsOf(kOrigin, "", 1).empty());
}

// A space is a cell the font inks nothing in.
// So is any character it has no glyph for.
TEST(TextRasterTest, ForEachGlyphPixel_VisitsNothingForABlankCell)
{
    EXPECT_TRUE(pixelsOf(kOrigin, " ", 1).empty());
    EXPECT_TRUE(pixelsOf(kOrigin, "\n\t\x7f", 2).empty());
}

// Every inked pixel of the cells is one visit, and no more.
TEST(TextRasterTest, ForEachGlyphPixel_VisitsEveryInkedPixelOnce)
{
    EXPECT_EQ(inkedPixels('A', 1), pixelsOf(kOrigin, "A", 1).size());
    EXPECT_EQ(
        inkedPixels('A', 2) + inkedPixels('B', 2),
        pixelsOf(kOrigin, "AB", 2).size());

    std::set<std::pair<std::int32_t, std::int32_t>> seen;

    for (const auto &visited : pixelsOf(kOrigin, "Wq8", 3))
    {
        EXPECT_TRUE(
            seen.emplace(visited.pixel.origin.x, visited.pixel.origin.y)
                .second);
    }
}

// The hard constraint of the whole arrangement, asserted directly.
// A glyph comes from a real font, and a cell is arithmetic.
// A font whose ink escaped its cell would move no metric at all.
// It would draw over the widget next door and be caught by nothing.
TEST(TextRasterTest, ForEachGlyphPixel_InksNothingOutsideTheMeasuredBox)
{
    constexpr std::string_view line =
        "Score: 1420 (Wq?) {[|]} @#$%&*_+~`^";

    for (std::uint32_t scale = 1; scale <= 4; ++scale)
    {
        const Size box = textSize(line, scale);

        for (const auto &visited : pixelsOf(kOrigin, line, scale))
        {
            EXPECT_GE(visited.pixel.origin.x, kOrigin.x);
            EXPECT_GE(visited.pixel.origin.y, kOrigin.y);
            EXPECT_LT(
                visited.pixel.origin.x,
                kOrigin.x + static_cast<std::int32_t>(box.width));
            EXPECT_LT(
                visited.pixel.origin.y,
                kOrigin.y + static_cast<std::int32_t>(box.height));
            EXPECT_EQ((Size{.width = 1, .height = 1}), visited.pixel.size);
        }
    }
}

// One cell along per character, at the advance the metrics promise.
TEST(TextRasterTest, ForEachGlyphPixel_AdvancesOneCellPerCharacter)
{
    constexpr std::uint32_t scale = 2;
    const auto alone = pixelsOf(kOrigin, "A", scale);
    const auto second = pixelsOf(
        Point{
            .x = kOrigin.x
                + static_cast<std::int32_t>(kGlyphAdvance * scale),
            .y = kOrigin.y},
        "A",
        scale);
    const auto pair = pixelsOf(kOrigin, " A", scale);

    ASSERT_EQ(second.size(), pair.size());

    // A leading space inks nothing, so the pair is the second cell.
    for (std::size_t index = 0; index < pair.size(); ++index)
    {
        EXPECT_EQ(second.at(index).pixel, pair.at(index).pixel);
    }

    EXPECT_EQ(alone.size(), pair.size());
}

// A bigger scale is a bigger cell, drawn from a bigger rasterisation.
// It is deliberately not one glyph with every pixel multiplied.
// That is what a bitmap font gave, and what made large text blocky.
TEST(TextRasterTest, ForEachGlyphPixel_DrawsIntoTheWholeScaledCell)
{
    const auto plain = pixelsOf(Point{}, "A", 1);
    const auto tripled = pixelsOf(Point{}, "A", 3);

    EXPECT_GT(tripled.size(), plain.size() * 3);

    for (const auto &visited : tripled)
    {
        EXPECT_LT(
            visited.pixel.origin.x,
            static_cast<std::int32_t>(kGlyphAdvance * 3));
        EXPECT_LT(
            visited.pixel.origin.y,
            static_cast<std::int32_t>(kGlyphLineHeight * 3));
    }
}

// Every visit is the cells' own answer, in reading order.
// The coverage arrives folded into the colour a backend fills with.
TEST(TextRasterTest, ForEachGlyphPixel_HandsOverTheCellsOwnCoverage)
{
    constexpr std::uint32_t scale = 2;
    constexpr std::string_view text = "gW";

    const auto &cells = glyphCells(scale);
    std::vector<Visited> expected;

    for (std::size_t cell = 0; cell < text.size(); ++cell)
    {
        for (std::uint32_t row = 0; row < cells.cellSize().height; ++row)
        {
            for (std::uint32_t column = 0;
                 column < cells.cellSize().width;
                 ++column)
            {
                const auto coverage =
                    cells.coverageAt(text.at(cell), column, row);

                if (coverage == 0)
                {
                    continue;
                }

                expected.push_back(Visited{
                    Rect{
                        .origin =
                            {.x = kOrigin.x
                                 + static_cast<std::int32_t>(
                                     (cell * kGlyphAdvance * scale)
                                     + column),
                             .y = kOrigin.y
                                 + static_cast<std::int32_t>(row)},
                        .size = {.width = 1, .height = 1}},
                    glyphPixelColor(kInk, coverage)});
            }
        }
    }

    const auto pixels = pixelsOf(kOrigin, text, scale);

    ASSERT_EQ(expected.size(), pixels.size());
    ASSERT_FALSE(expected.empty());

    for (std::size_t index = 0; index < expected.size(); ++index)
    {
        EXPECT_EQ(expected.at(index).pixel, pixels.at(index).pixel);
        EXPECT_EQ(expected.at(index).color, pixels.at(index).color);
    }
}

// An origin far enough out to wrap a 32-bit sum is worked out wide.
TEST(TextRasterTest, ForEachGlyphPixel_PlacesAFarOriginWithoutWrapping)
{
    constexpr Point far{.x = 2147483000, .y = 0};
    const auto pixels = pixelsOf(far, " A", 1);

    ASSERT_FALSE(pixels.empty());

    for (const auto &visited : pixels)
    {
        EXPECT_GE(visited.pixel.origin.x, far.x);
    }
}
