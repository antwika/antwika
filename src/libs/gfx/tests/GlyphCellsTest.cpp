#include <gtest/gtest.h>

#include <cstdint>

#include <antwika/gfx/GlyphCells.hpp>
#include <antwika/gfx/Glyphs.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/GlyphCellsCache.hpp>

using antwika::gfx::GlyphCells;
using antwika::gfx::GlyphCellsCache;
using antwika::gfx::kGlyphAdvance;
using antwika::gfx::kGlyphLineHeight;
using antwika::gfx::Size;

namespace
{
    [[nodiscard]] std::uint32_t inkedPixels(
        const GlyphCells &cells, char character)
    {
        std::uint32_t count = 0;

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

TEST(GlyphCellsTest, CellSize_IsTheFontMetricsTimesTheScale)
{
    EXPECT_EQ(
        (Size{.width = kGlyphAdvance, .height = kGlyphLineHeight}),
        GlyphCells{1}.cellSize());
    EXPECT_EQ(
        (Size{
            .width = kGlyphAdvance * 3,
            .height = kGlyphLineHeight * 3}),
        GlyphCells{3}.cellSize());
}

TEST(GlyphCellsTest, CellSize_IsNothingAtZeroScale)
{
    const GlyphCells cells{0};

    EXPECT_EQ((Size{}), cells.cellSize());
    EXPECT_EQ(0, cells.coverageAt('A', 0, 0));
}

TEST(GlyphCellsTest, CoverageAt_InksEveryPrintableCharacterButSpace)
{
    const GlyphCells cells{2};

    for (char character = '!'; character <= '~'; ++character)
    {
        EXPECT_GT(inkedPixels(cells, character), 0U) << character;
    }

    EXPECT_EQ(0U, inkedPixels(cells, ' '));
}

TEST(GlyphCellsTest, CoverageAt_IsBlankOutsideTheCoveredRange)
{
    const GlyphCells cells{2};

    EXPECT_EQ(0U, inkedPixels(cells, '\n'));
    EXPECT_EQ(0U, inkedPixels(cells, '\x7f'));
    EXPECT_EQ(0U, inkedPixels(cells, '\x80'));
    EXPECT_EQ(0U, inkedPixels(cells, static_cast<char>(-1)));
}

TEST(GlyphCellsTest, CoverageAt_IsBlankOutsideTheCell)
{
    const GlyphCells cells{2};
    const Size cellSize = cells.cellSize();

    EXPECT_EQ(0, cells.coverageAt('A', cellSize.width, 0));
    EXPECT_EQ(0, cells.coverageAt('A', 0, cellSize.height));
}

TEST(GlyphCellsTest, CoverageAt_IsBlankPastTheRightEdgeOfTheCell)
{
    const GlyphCells cells{2};
    const Size cellSize = cells.cellSize();
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
    const GlyphCells cells{2};
    const Size cellSize = cells.cellSize();
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
    const GlyphCells cells{2};
    bool partial = false;

    for (std::uint32_t row = 0; row < cells.cellSize().height; ++row)
    {
        for (std::uint32_t column = 0;
             column < cells.cellSize().width;
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
    const GlyphCells cells{2};

    ASSERT_EQ(cells.cellSize().width, 12U);
    ASSERT_EQ(cells.cellSize().height, 16U);

    unsigned long total = 0;
    unsigned long covered = 0;

    for (std::uint32_t row = 0; row < cells.cellSize().height; ++row)
    {
        for (std::uint32_t column = 0;
             column < cells.cellSize().width;
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

    const GlyphCells &firstCells = cache.at(2);
    const GlyphCells &againCells = cache.at(2);
    const GlyphCells &otherCells = cache.at(3);

    EXPECT_EQ(&firstCells, &againCells);
    EXPECT_NE(&firstCells, &otherCells);
    EXPECT_EQ(&firstCells, &cache.at(2));
    EXPECT_EQ(firstCells.cellSize(), againCells.cellSize());
    EXPECT_NE(firstCells.cellSize(), otherCells.cellSize());
}

TEST(GlyphCellsTest, GlyphCellsCache_AnswersAZeroScaleWithEmptyCells)
{
    GlyphCellsCache cache;

    EXPECT_EQ((Size{}), cache.at(0).cellSize());
}

