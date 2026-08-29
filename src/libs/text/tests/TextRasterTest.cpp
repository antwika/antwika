#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <set>
#include <string_view>
#include <utility>
#include <vector>

#include <antwika/gfx/Color.hpp>
#include <antwika/text/GlyphCells.hpp>
#include <antwika/gfx/Glyphs.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/text/TextLayout.hpp>
#include <antwika/text/TextRaster.hpp>
#include <antwika/text/GlyphCellsCache.hpp>

using antwika::gfx::Color;
using antwika::text::forEachGlyphPixel;
using antwika::text::GlyphCells;
using antwika::text::GlyphCellsCache;
using antwika::text::getGlyphPixelColor;
using antwika::gfx::kGlyphAdvance;
using antwika::gfx::kGlyphLineHeight;
using antwika::gfx::Point;
using antwika::gfx::Rect;
using antwika::gfx::Size;
using antwika::gfx::TextScale;
using antwika::text::getTextSize;

namespace
{
    constexpr Point kOriginPoint{.x = 10, .y = 20};
    constexpr Color kInkColor{
        .red = 200, .green = 100, .blue = 50, .alpha = 255};

    struct PixelSample final
    {
        Rect pixelRect;
        Color color;
    };

    [[nodiscard]] std::vector<PixelSample> pixelsOf(
        Point originPoint,
        std::string_view text,
        std::uint32_t scale,
        Color color = kInkColor)
    {
        GlyphCellsCache cache;
        std::vector<PixelSample> visitedPixels;

        forEachGlyphPixel(
            cache,
            originPoint,
            text,
            TextScale{.multiplier = scale},
            color,
            [&visitedPixels](Rect pixel, Color pixelColor) {
                visitedPixels.push_back(PixelSample{pixel, pixelColor});
            });

        return visitedPixels;
    }

    [[nodiscard]] std::size_t getInkedPixels(
        char character, std::uint32_t scale)
    {
        const GlyphCells cells{TextScale{.multiplier = scale}};
        std::size_t count = 0;

        for (std::uint32_t row = 0; row < cells.getCellSize().height; ++row)
        {
            for (std::uint32_t column = 0;
                 column < cells.getCellSize().width;
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
    EXPECT_EQ(kInkColor, getGlyphPixelColor(kInkColor, 255));
    EXPECT_EQ(
        (Color{
            .red = kInkColor.red,
            .green = kInkColor.green,
            .blue = kInkColor.blue,
            .alpha = 128}),
        getGlyphPixelColor(kInkColor, 128));
    EXPECT_EQ(0, getGlyphPixelColor(kInkColor, 0).alpha);
}

TEST(TextRasterTest, GlyphPixelColor_MultipliesRatherThanReplaces)
{
    constexpr Color faintColor{.red = 255, .alpha = 128};

    EXPECT_EQ(128, getGlyphPixelColor(faintColor, 255).alpha);
    EXPECT_EQ(64, getGlyphPixelColor(faintColor, 129).alpha);
}

TEST(TextRasterTest, ForEachGlyphPixel_VisitsNothingAtZeroScale)
{
    EXPECT_TRUE(pixelsOf(kOriginPoint, "A", 0).empty());
}

TEST(TextRasterTest, ForEachGlyphPixel_VisitsNothingForAnEmptyLine)
{
    EXPECT_TRUE(pixelsOf(kOriginPoint, "", 1).empty());
}

TEST(TextRasterTest, ForEachGlyphPixel_VisitsNothingForABlankCell)
{
    EXPECT_TRUE(pixelsOf(kOriginPoint, " ", 1).empty());
    EXPECT_TRUE(pixelsOf(kOriginPoint, "\n\t\x7f", 2).empty());
}

TEST(TextRasterTest, ForEachGlyphPixel_VisitsEveryInkedPixelOnce)
{
    ASSERT_GT(getInkedPixels('A', 1), 0U);
    EXPECT_EQ(getInkedPixels('A', 1), pixelsOf(kOriginPoint, "A", 1).size());
    EXPECT_EQ(
        getInkedPixels('A', 2) + getInkedPixels('B', 2),
        pixelsOf(kOriginPoint, "AB", 2).size());

    const auto crowdedPixels = pixelsOf(kOriginPoint, "Wq8", 3);
    std::set<std::pair<std::int32_t, std::int32_t>> seenCells;

    ASSERT_FALSE(crowdedPixels.empty());

    for (const auto &sample : crowdedPixels)
    {
        EXPECT_TRUE(
            seenCells.emplace(
                sample.pixelRect.originPoint.x,
                sample.pixelRect.originPoint.y)
                .second);
    }

    EXPECT_EQ(seenCells.size(), crowdedPixels.size());
}

TEST(TextRasterTest, ForEachGlyphPixel_InksNothingOutsideTheMeasuredBox)
{
    constexpr std::string_view line =
        "Score: 1420 (Wq?) {[|]} @#$%&*_+~`^";

    std::size_t inspectedCount = 0;

    for (std::uint32_t scale = 1; scale <= 4; ++scale)
    {
        const Size boxSize =
            getTextSize(line, TextScale{.multiplier = scale});

        for (const auto &sample : pixelsOf(kOriginPoint, line, scale))
        {
            ++inspectedCount;

            EXPECT_GE(sample.pixelRect.originPoint.x, kOriginPoint.x);
            EXPECT_GE(sample.pixelRect.originPoint.y, kOriginPoint.y);
            EXPECT_LT(
                sample.pixelRect.originPoint.x,
                kOriginPoint.x + static_cast<std::int32_t>(boxSize.width));
            EXPECT_LT(
                sample.pixelRect.originPoint.y,
                kOriginPoint.y + static_cast<std::int32_t>(boxSize.height));
            EXPECT_EQ((Size{.width = 1, .height = 1}), sample.pixelRect.size);
        }
    }

    EXPECT_GT(inspectedCount, 0U);
}

TEST(TextRasterTest, ForEachGlyphPixel_AdvancesOneCellPerCharacter)
{
    constexpr std::uint32_t scale = 2;
    const auto alone = pixelsOf(kOriginPoint, "A", scale);
    const auto second = pixelsOf(
        Point{
            .x = kOriginPoint.x
                + static_cast<std::int32_t>(kGlyphAdvance * scale),
            .y = kOriginPoint.y},
        "A",
        scale);
    const auto pair = pixelsOf(kOriginPoint, " A", scale);

    ASSERT_EQ(second.size(), pair.size());

    for (std::size_t index = 0; index < pair.size(); ++index)
    {
        EXPECT_EQ(second.at(index).pixelRect, pair.at(index).pixelRect);
    }

    EXPECT_EQ(alone.size(), pair.size());
}

TEST(TextRasterTest, ForEachGlyphPixel_DrawsIntoTheWholeScaledCell)
{
    const auto plain = pixelsOf(Point{}, "A", 1);
    const auto tripledPixels = pixelsOf(Point{}, "A", 3);

    EXPECT_GT(tripledPixels.size(), plain.size() * 3);

    for (const auto &sample : tripledPixels)
    {
        EXPECT_LT(
            sample.pixelRect.originPoint.x,
            static_cast<std::int32_t>(kGlyphAdvance * 3));
        EXPECT_LT(
            sample.pixelRect.originPoint.y,
            static_cast<std::int32_t>(kGlyphLineHeight * 3));
    }
}

TEST(TextRasterTest, ForEachGlyphPixel_HandsOverTheCellsOwnCoverage)
{
    constexpr std::uint32_t scale = 2;
    constexpr std::string_view text = "gW";

    const GlyphCells cells{TextScale{.multiplier = scale}};
    std::vector<PixelSample> expectedSamples;

    for (std::size_t cell = 0; cell < text.size(); ++cell)
    {
        for (std::uint32_t row = 0; row < cells.getCellSize().height; ++row)
        {
            for (std::uint32_t column = 0;
                 column < cells.getCellSize().width;
                 ++column)
            {
                const auto coverage =
                    cells.coverageAt(text.at(cell), column, row);

                if (coverage == 0)
                {
                    continue;
                }

                expectedSamples.push_back(PixelSample{
                    Rect{
                        .originPoint =
                            {.x = kOriginPoint.x
                                 + static_cast<std::int32_t>(
                                     (cell * kGlyphAdvance * scale)
                                     + column),
                             .y = kOriginPoint.y
                                 + static_cast<std::int32_t>(row)},
                        .size = {.width = 1, .height = 1}},
                    getGlyphPixelColor(kInkColor, coverage)});
            }
        }
    }

    const auto pixels = pixelsOf(kOriginPoint, text, scale);

    ASSERT_EQ(expectedSamples.size(), pixels.size());
    ASSERT_FALSE(expectedSamples.empty());

    for (std::size_t index = 0; index < expectedSamples.size(); ++index)
    {
        EXPECT_EQ(
            expectedSamples.at(index).pixelRect,
            pixels.at(index).pixelRect);
        EXPECT_EQ(expectedSamples.at(index).color, pixels.at(index).color);
    }
}

TEST(TextRasterTest, ForEachGlyphPixel_PlacesAFarOriginWithoutWrapping)
{
    constexpr Point farPoint{.x = 2147483000, .y = 0};
    const auto pixels = pixelsOf(farPoint, " A", 1);

    ASSERT_FALSE(pixels.empty());

    for (const auto &sample : pixels)
    {
        EXPECT_GE(sample.pixelRect.originPoint.x, farPoint.x);
    }
}
