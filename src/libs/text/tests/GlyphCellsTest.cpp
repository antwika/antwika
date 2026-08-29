#include <gtest/gtest.h>

#include <cstdint>

#include <antwika/text/GlyphCells.hpp>
#include <antwika/gfx/Glyphs.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/text/GlyphCellsCache.hpp>

using antwika::text::GlyphCells;
using antwika::text::GlyphCellsCache;
using antwika::gfx::kGlyphAdvance;
using antwika::gfx::kGlyphLineHeight;
using antwika::gfx::Size;
using antwika::gfx::TextScale;

namespace
{
    [[nodiscard]] std::uint32_t getInkedPixels(
        const GlyphCells &cells, char character)
    {
        std::uint32_t count = 0;

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

TEST(GlyphCellsTest, CellSize_IsTheFontMetricsTimesTheScale)
{
    EXPECT_EQ(
        (Size{.width = kGlyphAdvance, .height = kGlyphLineHeight}),
        GlyphCells{TextScale{.multiplier = 1}}.getCellSize());
    EXPECT_EQ(
        (Size{
            .width = kGlyphAdvance * 3,
            .height = kGlyphLineHeight * 3}),
        GlyphCells{TextScale{.multiplier = 3}}.getCellSize());
}

TEST(GlyphCellsTest, CellSize_IsNothingAtZeroScale)
{
    const GlyphCells cells{TextScale{.multiplier = 0}};

    EXPECT_EQ((Size{}), cells.getCellSize());
    EXPECT_EQ(0, cells.coverageAt('A', 0, 0));
}

TEST(GlyphCellsTest, CoverageAt_InksEveryPrintableCharacterButSpace)
{
    const GlyphCells cells{TextScale{.multiplier = 2}};

    for (char character = '!'; character <= '~'; ++character)
    {
        EXPECT_GT(getInkedPixels(cells, character), 0U) << character;
    }

    EXPECT_EQ(0U, getInkedPixels(cells, ' '));
}

TEST(GlyphCellsTest, CoverageAt_IsBlankOutsideTheCoveredRange)
{
    const GlyphCells cells{TextScale{.multiplier = 2}};

    EXPECT_EQ(0U, getInkedPixels(cells, '\n'));
    EXPECT_EQ(0U, getInkedPixels(cells, '\x7f'));
    EXPECT_EQ(0U, getInkedPixels(cells, '\x80'));
    EXPECT_EQ(0U, getInkedPixels(cells, static_cast<char>(-1)));
}

TEST(GlyphCellsTest, CoverageAt_IsBlankOutsideTheCell)
{
    const GlyphCells cells{TextScale{.multiplier = 2}};
    const Size cellSize = cells.getCellSize();

    EXPECT_EQ(0, cells.coverageAt('A', cellSize.width, 0));
    EXPECT_EQ(0, cells.coverageAt('A', 0, cellSize.height));
}

TEST(GlyphCellsTest, CoverageAt_IsBlankPastTheRightEdgeOfTheCell)
{
    const GlyphCells cells{TextScale{.multiplier = 2}};
    const Size cellSize = cells.getCellSize();
    std::uint32_t inkOnTheRowBelow = 0;
    std::uint32_t inkPastTheEdge = 0;

    for (std::uint32_t row = 0; row + 1 < cellSize.height; ++row)
    {
        for (std::uint32_t column = 0; column < cellSize.width; ++column)
        {
            inkOnTheRowBelow +=
                cells.coverageAt('A', column, row + 1) != 0 ? 1U : 0U;

            inkPastTheEdge +=
                cells.coverageAt('A', cellSize.width + column, row) != 0
                ? 1U
                : 0U;
        }
    }

    ASSERT_GT(inkOnTheRowBelow, 0U);
    EXPECT_EQ(inkPastTheEdge, 0U);
}

TEST(GlyphCellsTest, CoverageAt_IsBlankBelowTheCell)
{
    const GlyphCells cells{TextScale{.multiplier = 2}};
    const Size cellSize = cells.getCellSize();
    std::uint32_t inkOnTheNextGlyph = 0;
    std::uint32_t inkBelowTheCell = 0;

    for (std::uint32_t row = 0; row < cellSize.height; ++row)
    {
        for (std::uint32_t column = 0; column < cellSize.width; ++column)
        {
            inkOnTheNextGlyph +=
                cells.coverageAt('B', column, row) != 0 ? 1U : 0U;

            inkBelowTheCell +=
                cells.coverageAt('A', column, cellSize.height + row) != 0
                ? 1U
                : 0U;
        }
    }

    ASSERT_GT(inkOnTheNextGlyph, 0U);
    EXPECT_EQ(inkBelowTheCell, 0U);
}

TEST(GlyphCellsTest, CoverageAt_ReportsPartialInkAtAGlyphsEdge)
{
    const GlyphCells cells{TextScale{.multiplier = 2}};
    bool partial = false;

    for (std::uint32_t row = 0; row < cells.getCellSize().height; ++row)
    {
        for (std::uint32_t column = 0;
             column < cells.getCellSize().width;
             ++column)
        {
            const auto coverage = cells.coverageAt('S', column, row);
            partial = partial || (coverage > 0 && coverage < 255);
        }
    }

    EXPECT_TRUE(partial);
}

TEST(GlyphCellsTest, GlyphCells_RastersTheRecordedCoverageForALetter)
{
    const GlyphCells cells{TextScale{.multiplier = 2}};

    ASSERT_EQ(cells.getCellSize().width, 12U);
    ASSERT_EQ(cells.getCellSize().height, 16U);

    unsigned long total = 0;
    unsigned long covered = 0;

    for (std::uint32_t row = 0; row < cells.getCellSize().height; ++row)
    {
        for (std::uint32_t column = 0;
             column < cells.getCellSize().width;
             ++column)
        {
            const auto value = cells.coverageAt('g', column, row);
            total += value;
            covered += (value > 0) ? 1U : 0U;
        }
    }

    EXPECT_EQ(covered, 52UL);
    EXPECT_EQ(total, 6036UL);
}

TEST(GlyphCellsTest, GlyphCellsCache_KeepsOneSetOfCellsPerScale)
{
    GlyphCellsCache cache;

    const GlyphCells &firstCells = cache.at(TextScale{.multiplier = 2});
    const GlyphCells &againCells = cache.at(TextScale{.multiplier = 2});
    const GlyphCells &otherCells = cache.at(TextScale{.multiplier = 3});

    EXPECT_EQ(&firstCells, &againCells);
    EXPECT_NE(&firstCells, &otherCells);
    EXPECT_EQ(&firstCells, &cache.at(TextScale{.multiplier = 2}));
    EXPECT_EQ(firstCells.getCellSize(), againCells.getCellSize());
    EXPECT_NE(firstCells.getCellSize(), otherCells.getCellSize());
}

TEST(GlyphCellsTest, GlyphCellsCache_AnswersAZeroScaleWithEmptyCells)
{
    GlyphCellsCache cache;

    EXPECT_EQ((Size{}), cache.at(TextScale{.multiplier = 0}).getCellSize());
}

