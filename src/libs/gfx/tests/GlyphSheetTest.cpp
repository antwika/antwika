#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <map>
#include <string_view>
#include <vector>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/GlyphBlit.hpp>
#include <antwika/gfx/GlyphCells.hpp>
#include <antwika/gfx/GlyphSheet.hpp>
#include <antwika/gfx/Glyphs.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/TextRaster.hpp>

using antwika::gfx::Bitmap;
using antwika::gfx::Color;
using antwika::gfx::forEachGlyphPixel;
using antwika::gfx::GlyphBlit;
using antwika::gfx::GlyphCells;
using antwika::gfx::GlyphCellsCache;
using antwika::gfx::glyphPixelColor;
using antwika::gfx::glyphSheetBitmap;
using antwika::gfx::glyphSheetBlits;
using antwika::gfx::glyphSheetCell;
using antwika::gfx::kBytesPerPixel;
using antwika::gfx::kGlyphCount;
using antwika::gfx::Point;
using antwika::gfx::Rect;
using antwika::gfx::Size;

namespace
{
    constexpr std::uint32_t kScale = 2;

    constexpr Point kOrigin{.x = 10, .y = 20};

    constexpr Color kInk{
        .red = 200, .green = 100, .blue = 50, .alpha = 255};

    using Painted = std::map<std::pair<std::int32_t, std::int32_t>, Color>;

    [[nodiscard]] Painted paintedByPixels(
        std::string_view text, Color ink)
    {
        GlyphCellsCache cache;
        Painted painted;

        forEachGlyphPixel(
            cache,
            kOrigin,
            text,
            kScale,
            ink,
            [&painted](Rect pixel, Color color) {
                painted[{pixel.origin.x, pixel.origin.y}] = color;
            });

        return painted;
    }

    [[nodiscard]] Painted paintedBySheet(
        std::string_view text, Color ink)
    {
        GlyphCellsCache cache;
        const GlyphCells &cells = cache.at(kScale);
        const Bitmap sheet = glyphSheetBitmap(cells);

        Painted painted;

        for (const GlyphBlit &blit :
             glyphSheetBlits(cells, kOrigin, text))
        {
            for (std::uint32_t row = 0; row < blit.source.size.height;
                 ++row)
            {
                for (std::uint32_t column = 0;
                     column < blit.source.size.width;
                     ++column)
                {
                    const auto x = static_cast<std::size_t>(
                        blit.source.origin.x + column);
                    const auto y = static_cast<std::size_t>(
                        blit.source.origin.y + row);
                    const auto at =
                        ((y * sheet.size.width) + x) * kBytesPerPixel;

                    const auto coverage = sheet.pixels[at + 3];

                    if (coverage == 0)
                    {
                        continue;
                    }

                    const Color modulated{
                        .red = static_cast<std::uint8_t>(
                            sheet.pixels[at] * ink.red / 255),
                        .green = static_cast<std::uint8_t>(
                            sheet.pixels[at + 1] * ink.green / 255),
                        .blue = static_cast<std::uint8_t>(
                            sheet.pixels[at + 2] * ink.blue / 255),
                        .alpha = static_cast<std::uint8_t>(
                            coverage * ink.alpha / 255)};

                    painted[{
                        blit.destination.origin.x
                            + static_cast<std::int32_t>(column),
                        blit.destination.origin.y
                            + static_cast<std::int32_t>(row)}] = modulated;
                }
            }
        }

        return painted;
    }
}

TEST(GlyphSheetTest, GlyphSheetBitmap_HoldsEveryGlyphInOneRow)
{
    GlyphCellsCache cache;
    const GlyphCells &cells = cache.at(kScale);

    const auto sheet = glyphSheetBitmap(cells);

    EXPECT_EQ(
        sheet.size,
        (Size{
            .width = static_cast<std::uint32_t>(kGlyphCount)
                * cells.cellSize().width,
            .height = cells.cellSize().height}));
    EXPECT_TRUE(sheet.isComplete());
}

TEST(GlyphSheetTest, GlyphSheetBitmap_CarriesEveryGlyphsCoverageOverWhite)
{
    GlyphCellsCache cache;
    const GlyphCells &cells = cache.at(kScale);
    const Size cell = cells.cellSize();

    const auto sheet = glyphSheetBitmap(cells);

    std::size_t inked = 0;

    for (std::size_t index = 0; index < kGlyphCount; ++index)
    {
        const auto character = static_cast<char>(
            static_cast<unsigned char>(antwika::gfx::kFirstGlyph + index));

        for (std::uint32_t row = 0; row < cell.height; ++row)
        {
            for (std::uint32_t column = 0; column < cell.width; ++column)
            {
                const auto at =
                    ((static_cast<std::size_t>(row) * sheet.size.width)
                     + (index * cell.width) + column)
                    * kBytesPerPixel;

                ASSERT_EQ(sheet.pixels[at], 255);
                ASSERT_EQ(sheet.pixels[at + 1], 255);
                ASSERT_EQ(sheet.pixels[at + 2], 255);
                ASSERT_EQ(
                    sheet.pixels[at + 3],
                    cells.coverageAt(character, column, row));

                if (sheet.pixels[at + 3] != 0)
                {
                    ++inked;
                }
            }
        }
    }

    EXPECT_GT(inked, 0U);
}

TEST(GlyphSheetTest, GlyphSheetBitmap_InksTheLastGlyphAsWellAsTheFirst)
{
    GlyphCellsCache cache;
    const GlyphCells &cells = cache.at(kScale);
    const Size cell = cells.cellSize();

    const auto sheet = glyphSheetBitmap(cells);

    const auto lastCell = glyphSheetCell(
        cells, static_cast<char>(antwika::gfx::kLastGlyph));

    ASSERT_TRUE(lastCell.has_value());

    std::size_t inked = 0;

    for (std::uint32_t row = 0; row < cell.height; ++row)
    {
        for (std::uint32_t column = 0; column < cell.width; ++column)
        {
            const auto at =
                ((static_cast<std::size_t>(row) * sheet.size.width)
                 + static_cast<std::size_t>(lastCell->origin.x) + column)
                * kBytesPerPixel;

            if (sheet.pixels[at + 3] != 0)
            {
                ++inked;
            }
        }
    }

    EXPECT_GT(inked, 0U);
}

TEST(GlyphSheetTest, GlyphSheetCell_PlacesGlyphsAtTheirCodepointOffset)
{
    GlyphCellsCache cache;
    const GlyphCells &cells = cache.at(kScale);
    const Size cell = cells.cellSize();

    EXPECT_EQ(
        glyphSheetCell(cells, ' '),
        (Rect{.origin = {.x = 0, .y = 0}, .size = cell}));
    EXPECT_EQ(
        glyphSheetCell(cells, '!'),
        (Rect{
            .origin = {.x = static_cast<std::int32_t>(cell.width),
                       .y = 0},
            .size = cell}));
}

TEST(GlyphSheetTest, GlyphSheetCell_HoldsNoGlyphOutsideThePrintableRange)
{
    GlyphCellsCache cache;
    const GlyphCells &cells = cache.at(kScale);

    EXPECT_FALSE(glyphSheetCell(cells, '\n').has_value());
    EXPECT_FALSE(
        glyphSheetCell(cells, static_cast<char>(0x80)).has_value());
}

TEST(GlyphSheetTest, GlyphSheetBlits_StepsOneCellPerCharacter)
{
    GlyphCellsCache cache;
    const GlyphCells &cells = cache.at(kScale);
    const Size cell = cells.cellSize();

    const auto blits = glyphSheetBlits(cells, kOrigin, "AB");

    ASSERT_EQ(blits.size(), 2U);
    EXPECT_EQ(blits[0].destination.origin, kOrigin);
    EXPECT_EQ(
        blits[1].destination.origin,
        (Point{
            .x = kOrigin.x + static_cast<std::int32_t>(cell.width),
            .y = kOrigin.y}));
    EXPECT_EQ(blits[0].destination.size, cell);
}

TEST(GlyphSheetTest, GlyphSheetBlits_KeepsTheSpacingOfACharacterItSkips)
{
    GlyphCellsCache cache;
    const GlyphCells &cells = cache.at(kScale);
    const Size cell = cells.cellSize();

    const auto blits = glyphSheetBlits(cells, kOrigin, "\nB");

    ASSERT_EQ(blits.size(), 1U);
    EXPECT_EQ(
        blits[0].destination.origin,
        (Point{
            .x = kOrigin.x + static_cast<std::int32_t>(cell.width),
            .y = kOrigin.y}));
}

TEST(GlyphSheetTest, GlyphSheetBlits_DrawNothingForAnEmptyLine)
{
    GlyphCellsCache cache;

    EXPECT_TRUE(glyphSheetBlits(cache.at(kScale), kOrigin, "").empty());
}

TEST(GlyphSheetTest, GlyphSheet_PaintsWhatThePerPixelPathPainted)
{
    const auto text = "Fire 42%";

    const auto byPixels = paintedByPixels(text, kInk);
    const auto bySheet = paintedBySheet(text, kInk);

    ASSERT_FALSE(byPixels.empty());
    EXPECT_EQ(bySheet, byPixels);
}

TEST(GlyphSheetTest, GlyphSheet_CarriesAFadedInkThroughUnchanged)
{
    constexpr Color kFaded{
        .red = 200, .green = 100, .blue = 50, .alpha = 128};

    const auto byPixels = paintedByPixels("Aj", kFaded);
    const auto bySheet = paintedBySheet("Aj", kFaded);

    ASSERT_FALSE(byPixels.empty());
    EXPECT_EQ(bySheet, byPixels);
}
