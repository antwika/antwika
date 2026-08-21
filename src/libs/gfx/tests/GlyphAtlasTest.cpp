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
#include <antwika/gfx/GlyphAtlas.hpp>
#include <antwika/gfx/Glyphs.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/TextRaster.hpp>
#include <antwika/gfx/GlyphCellsCache.hpp>

using antwika::gfx::Bitmap;
using antwika::gfx::Color;
using antwika::gfx::forEachGlyphPixel;
using antwika::gfx::GlyphBlit;
using antwika::gfx::GlyphCells;
using antwika::gfx::GlyphCellsCache;
using antwika::gfx::glyphPixelColor;
using antwika::gfx::glyphAtlasBitmap;
using antwika::gfx::glyphAtlasBlits;
using antwika::gfx::glyphAtlasCell;
using antwika::gfx::kBytesPerPixel;
using antwika::gfx::kGlyphCount;
using antwika::gfx::Point;
using antwika::gfx::Rect;
using antwika::gfx::Size;

namespace
{
    constexpr std::uint32_t kScale = 2;

    constexpr Point kOriginPoint{.x = 10, .y = 20};

    constexpr Color kInkColor{
        .red = 200, .green = 100, .blue = 50, .alpha = 255};

    using PaintRecord = std::map<std::pair<std::int32_t, std::int32_t>, Color>;

    [[nodiscard]] PaintRecord paintedByPixels(
        std::string_view text, Color inkColor)
    {
        GlyphCellsCache cache;
        PaintRecord paintedCells;

        forEachGlyphPixel(
            cache,
            kOriginPoint,
            text,
            kScale,
            inkColor,
            [&paintedCells](Rect pixel, Color color) {
                paintedCells[{pixel.originPoint.x, pixel.originPoint.y}] =
                color;
            });

        return paintedCells;
    }

    [[nodiscard]] PaintRecord paintedByAtlas(
        std::string_view text, Color inkColor)
    {
        GlyphCellsCache cache;
        const GlyphCells &cells = cache.at(kScale);
        const Bitmap atlasBitmap = glyphAtlasBitmap(cells);

        PaintRecord paintedCells;

        for (const GlyphBlit &blit :
             glyphAtlasBlits(cells, kOriginPoint, text))
        {
            for (std::uint32_t row = 0; row < blit.sourceRect.size.height;
                 ++row)
            {
                for (std::uint32_t column = 0;
                     column < blit.sourceRect.size.width;
                     ++column)
                {
                    const auto x = static_cast<std::size_t>(
                        blit.sourceRect.originPoint.x + column);
                    const auto y = static_cast<std::size_t>(
                        blit.sourceRect.originPoint.y + row);
                    const auto byteIndex =
                        ((y * atlasBitmap.size.width) + x) * kBytesPerPixel;

                    const auto coverage = atlasBitmap.pixels[byteIndex + 3];

                    if (coverage == 0)
                    {
                        continue;
                    }

                    const Color modulatedColor{
                        .red = static_cast<std::uint8_t>(
                            atlasBitmap.pixels[byteIndex] * inkColor.red / 255),
                        .green = static_cast<std::uint8_t>(
                            atlasBitmap.pixels[
                            byteIndex + 1] * inkColor.green / 255),
                        .blue = static_cast<std::uint8_t>(
                            atlasBitmap.pixels[
                            byteIndex + 2] * inkColor.blue / 255),
                        .alpha = static_cast<std::uint8_t>(
                            coverage * inkColor.alpha / 255)};

                    paintedCells[{
                        blit.destinationRect.originPoint.x
                            + static_cast<std::int32_t>(column),
                        blit.destinationRect.originPoint.y
                            + static_cast<std::int32_t>(row)}] = modulatedColor;
                }
            }
        }

        return paintedCells;
    }
}

TEST(GlyphAtlasTest, GlyphAtlasBitmap_HoldsEveryGlyphInOneRow)
{
    GlyphCellsCache cache;
    const GlyphCells &cells = cache.at(kScale);

    const auto atlas = glyphAtlasBitmap(cells);

    EXPECT_EQ(
        atlas.size,
        (Size{
            .width = static_cast<std::uint32_t>(kGlyphCount)
                * cells.cellSize().width,
            .height = cells.cellSize().height}));
    EXPECT_TRUE(atlas.isValid());
}

TEST(GlyphAtlasTest, GlyphAtlasBitmap_CarriesEveryGlyphsCoverageOverWhite)
{
    GlyphCellsCache cache;
    const GlyphCells &cells = cache.at(kScale);
    const Size cellSize = cells.cellSize();

    const auto atlas = glyphAtlasBitmap(cells);

    std::size_t inkedCount = 0;

    for (std::size_t index = 0; index < kGlyphCount; ++index)
    {
        const auto character = static_cast<char>(
            static_cast<unsigned char>(antwika::gfx::kFirstGlyph + index));

        for (std::uint32_t row = 0; row < cellSize.height; ++row)
        {
            for (std::uint32_t column = 0; column < cellSize.width; ++column)
            {
                const auto byteIndex =
                    ((static_cast<std::size_t>(row) * atlas.size.width)
                     + (index * cellSize.width) + column)
                    * kBytesPerPixel;

                ASSERT_EQ(atlas.pixels[byteIndex], 255);
                ASSERT_EQ(atlas.pixels[byteIndex + 1], 255);
                ASSERT_EQ(atlas.pixels[byteIndex + 2], 255);
                ASSERT_EQ(
                    atlas.pixels[byteIndex + 3],
                    cells.coverageAt(character, column, row));

                if (atlas.pixels[byteIndex + 3] != 0)
                {
                    ++inkedCount;
                }
            }
        }
    }

    EXPECT_GT(inkedCount, 0U);
}

TEST(GlyphAtlasTest, GlyphAtlasBitmap_InksTheLastGlyphAsWellAsTheFirst)
{
    GlyphCellsCache cache;
    const GlyphCells &cells = cache.at(kScale);
    const Size cellSize = cells.cellSize();

    const auto atlas = glyphAtlasBitmap(cells);

    const auto lastCell = glyphAtlasCell(
        cells, static_cast<char>(antwika::gfx::kLastGlyph));

    ASSERT_TRUE(lastCell.has_value());

    std::size_t inkedCount = 0;

    for (std::uint32_t row = 0; row < cellSize.height; ++row)
    {
        for (std::uint32_t column = 0; column < cellSize.width; ++column)
        {
            const auto byteIndex =
                ((static_cast<std::size_t>(row) * atlas.size.width)
                 + static_cast<std::size_t>(lastCell->originPoint.x) + column)
                * kBytesPerPixel;

            if (atlas.pixels[byteIndex + 3] != 0)
            {
                ++inkedCount;
            }
        }
    }

    EXPECT_GT(inkedCount, 0U);
}

TEST(GlyphAtlasTest, GlyphAtlasCell_PlacesGlyphsAtTheirCodepointOffset)
{
    GlyphCellsCache cache;
    const GlyphCells &cells = cache.at(kScale);
    const Size cellSize = cells.cellSize();

    EXPECT_EQ(
        glyphAtlasCell(cells, ' '),
        (Rect{.originPoint = {.x = 0, .y = 0}, .size = cellSize}));
    EXPECT_EQ(
        glyphAtlasCell(cells, '!'),
        (Rect{
            .originPoint = {.x = static_cast<std::int32_t>(cellSize.width),
                       .y = 0},
            .size = cellSize}));
}

TEST(GlyphAtlasTest, GlyphAtlasCell_HoldsNoGlyphOutsideThePrintableRange)
{
    GlyphCellsCache cache;
    const GlyphCells &cells = cache.at(kScale);

    EXPECT_FALSE(glyphAtlasCell(cells, '\n').has_value());
    EXPECT_FALSE(
        glyphAtlasCell(cells, static_cast<char>(0x80)).has_value());
}

TEST(GlyphAtlasTest, GlyphAtlasBlits_StepsOneCellPerCharacter)
{
    GlyphCellsCache cache;
    const GlyphCells &cells = cache.at(kScale);
    const Size cellSize = cells.cellSize();

    const auto blits = glyphAtlasBlits(cells, kOriginPoint, "AB");

    ASSERT_EQ(blits.size(), 2U);
    EXPECT_EQ(blits[0].destinationRect.originPoint, kOriginPoint);
    EXPECT_EQ(
        blits[1].destinationRect.originPoint,
        (Point{
            .x = kOriginPoint.x + static_cast<std::int32_t>(cellSize.width),
            .y = kOriginPoint.y}));
    EXPECT_EQ(blits[0].destinationRect.size, cellSize);
}

TEST(GlyphAtlasTest, GlyphAtlasBlits_KeepsTheSpacingOfACharacterItSkips)
{
    GlyphCellsCache cache;
    const GlyphCells &cells = cache.at(kScale);
    const Size cellSize = cells.cellSize();

    const auto blits = glyphAtlasBlits(cells, kOriginPoint, "\nB");

    ASSERT_EQ(blits.size(), 1U);
    EXPECT_EQ(
        blits[0].destinationRect.originPoint,
        (Point{
            .x = kOriginPoint.x + static_cast<std::int32_t>(cellSize.width),
            .y = kOriginPoint.y}));
}

TEST(GlyphAtlasTest, GlyphAtlasBlits_DrawNothingForAnEmptyLine)
{
    GlyphCellsCache cache;

    EXPECT_TRUE(glyphAtlasBlits(cache.at(kScale), kOriginPoint, "").empty());
}

TEST(GlyphAtlasTest, GlyphAtlas_PaintsWhatThePerPixelPathPainted)
{
    const auto text = "Fire 42%";

    const auto byPixels = paintedByPixels(text, kInkColor);
    const auto byAtlas = paintedByAtlas(text, kInkColor);

    ASSERT_FALSE(byPixels.empty());
    EXPECT_EQ(byAtlas, byPixels);
}

TEST(GlyphAtlasTest, GlyphAtlas_CarriesAFadedInkThroughUnchanged)
{
    constexpr Color kFadedColor{
        .red = 200, .green = 100, .blue = 50, .alpha = 128};

    const auto byPixels = paintedByPixels("Aj", kFadedColor);
    const auto byAtlas = paintedByAtlas("Aj", kFadedColor);

    ASSERT_FALSE(byPixels.empty());
    EXPECT_EQ(byAtlas, byPixels);
}
