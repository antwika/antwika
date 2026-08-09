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
using antwika::gfx::GlyphCells;
using antwika::gfx::GlyphCellsCache;
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

    struct Visited final
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
        GlyphCellsCache cache;
        std::vector<Visited> pixels;

        forEachGlyphPixel(
            cache,
            origin,
            text,
            scale,
            color,
            [&pixels](Rect pixel, Color pixelColor) {
                pixels.push_back(Visited{pixel, pixelColor});
            });

        return pixels;
    }

    [[nodiscard]] std::size_t inkedPixels(
        char character, std::uint32_t scale)
    {
        const GlyphCells cells{scale};
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
}

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

TEST(TextRasterTest, ForEachGlyphPixel_VisitsNothingForABlankCell)
{
    EXPECT_TRUE(pixelsOf(kOrigin, " ", 1).empty());
    EXPECT_TRUE(pixelsOf(kOrigin, "\n\t\x7f", 2).empty());
}

TEST(TextRasterTest, ForEachGlyphPixel_VisitsEveryInkedPixelOnce)
{
    ASSERT_GT(inkedPixels('A', 1), 0U);
    EXPECT_EQ(inkedPixels('A', 1), pixelsOf(kOrigin, "A", 1).size());
    EXPECT_EQ(
        inkedPixels('A', 2) + inkedPixels('B', 2),
        pixelsOf(kOrigin, "AB", 2).size());

    const auto crowded = pixelsOf(kOrigin, "Wq8", 3);
    std::set<std::pair<std::int32_t, std::int32_t>> seen;

    ASSERT_FALSE(crowded.empty());

    for (const auto &visited : crowded)
    {
        EXPECT_TRUE(
            seen.emplace(visited.pixel.origin.x, visited.pixel.origin.y)
                .second);
    }

    EXPECT_EQ(seen.size(), crowded.size());
}

TEST(TextRasterTest, ForEachGlyphPixel_InksNothingOutsideTheMeasuredBox)
{
    constexpr std::string_view line =
        "Score: 1420 (Wq?) {[|]} @#$%&*_+~`^";

    std::size_t inspected = 0;

    for (std::uint32_t scale = 1; scale <= 4; ++scale)
    {
        const Size box = textSize(line, scale);

        for (const auto &visited : pixelsOf(kOrigin, line, scale))
        {
            ++inspected;

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

    EXPECT_GT(inspected, 0U);
}

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

    for (std::size_t index = 0; index < pair.size(); ++index)
    {
        EXPECT_EQ(second.at(index).pixel, pair.at(index).pixel);
    }

    EXPECT_EQ(alone.size(), pair.size());
}

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

TEST(TextRasterTest, ForEachGlyphPixel_HandsOverTheCellsOwnCoverage)
{
    constexpr std::uint32_t scale = 2;
    constexpr std::string_view text = "gW";

    const GlyphCells cells{scale};
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
