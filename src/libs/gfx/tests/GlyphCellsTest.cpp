#include <gtest/gtest.h>

#include <cstdint>

#include <antwika/gfx/GlyphCells.hpp>
#include <antwika/gfx/Glyphs.hpp>
#include <antwika/gfx/Size.hpp>

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
} // namespace

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

// A zero scale draws nothing, so it rasterises nothing either.
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
        EXPECT_GT(inkedPixels(cells, character), 0U)
            << "nothing drawn for " << character;
    }

    EXPECT_EQ(0U, inkedPixels(cells, ' '));
}

// A character outside the range covered is a blank cell.
// It is the same width as any other, and never a refusal or a box.
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
    const Size cell = cells.cellSize();

    EXPECT_EQ(0, cells.coverageAt('A', cell.width, 0));
    EXPECT_EQ(0, cells.coverageAt('A', 0, cell.height));
}

// The glyphs are outlines rather than a table of bits.
// So a pixel at the edge of one holds part of a pixel's worth of ink.
// A backend draws that as a fraction of the colour's own alpha.
// That is what makes small text legible instead of speckled.
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

// Two builds of one scale agree pixel for pixel.
// That is what lets a cache of them be a memo rather than a decision.
TEST(GlyphCellsTest, GlyphCells_BuildsTheSameCellsEveryTime)
{
    const GlyphCells first{2};
    const GlyphCells second{2};

    for (std::uint32_t row = 0; row < first.cellSize().height; ++row)
    {
        for (std::uint32_t column = 0;
             column < first.cellSize().width;
             ++column)
        {
            ASSERT_EQ(
                first.coverageAt('g', column, row),
                second.coverageAt('g', column, row));
        }
    }
}

// A cache hands back the same cells rather than building them again.
// It keeps one set per scale, and a later scale moves no earlier one.
TEST(GlyphCellsTest, GlyphCellsCache_KeepsOneSetOfCellsPerScale)
{
    GlyphCellsCache cache;

    const GlyphCells &first = cache.at(2);
    const GlyphCells &again = cache.at(2);
    const GlyphCells &other = cache.at(3);

    EXPECT_EQ(&first, &again);
    EXPECT_NE(&first, &other);
    EXPECT_EQ(&first, &cache.at(2));
    EXPECT_EQ(first.cellSize(), again.cellSize());
    EXPECT_NE(first.cellSize(), other.cellSize());
}

TEST(GlyphCellsTest, GlyphCellsCache_AnswersAZeroScaleWithEmptyCells)
{
    GlyphCellsCache cache;

    EXPECT_EQ((Size{}), cache.at(0).cellSize());
}
