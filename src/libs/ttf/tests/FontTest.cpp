#include "antwika/ttf/Font.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

#include "antwika/ttf/FontMetrics.hpp"
#include "antwika/ttf/Glyph.hpp"
#include "antwika/ttf/GlyphMetrics.hpp"
#include "antwika/ttf/TtfError.hpp"

#include "SyntheticFont.hpp"

using antwika::ttf::Font;
using antwika::ttf::FontMetrics;
using antwika::ttf::Glyph;
using antwika::ttf::GlyphMetrics;
using antwika::ttf::TtfError;
using antwika::ttf::tests::buildDirectory;
using antwika::ttf::tests::buildFont;
using antwika::ttf::tests::FontRecipe;
using antwika::ttf::tests::TableRecord;

namespace
{
    // Every measurement below is at this height.
    // The font's 1000 units per em then scale by exactly a fiftieth.
    constexpr std::uint32_t kHeight = 20;

    // Room after a 28-byte offset table for a record to point into.
    constexpr std::size_t kRoom = 64;

    std::vector<std::uint8_t> directoryWithOneTable(
        std::uint32_t flavour)
    {
        const std::array<TableRecord, 1> records{
            TableRecord{.tag = "head", .offset = 28, .length = 4}};

        return buildDirectory(flavour, 1, records, kRoom);
    }
} // namespace

TEST(FontTest, ReadsAWholeFont)
{
    EXPECT_NO_THROW((void)Font{buildFont()});
}

// The other flavour a TrueType-outline font opens with.
// Apple wrote it and it is still on disk in the wild.
TEST(FontTest, ReadsTheAppleFlavour)
{
    EXPECT_NO_THROW(
        (void)Font{buildFont(FontRecipe{.flavour = 0x74727565})});
}

TEST(FontTest, RefusesBytesTooShortForAnOffsetTable)
{
    EXPECT_THROW((void)Font{std::vector<std::uint8_t>(4, 0)}, TtfError);
}

TEST(FontTest, RefusesAFontCollection)
{
    EXPECT_THROW(
        (void)Font{directoryWithOneTable(0x74746366)}, TtfError);
}

TEST(FontTest, RefusesCffOutlines)
{
    EXPECT_THROW(
        (void)Font{directoryWithOneTable(0x4F54544F)}, TtfError);
}

// The message names the four bytes, printable or not.
// Somebody holding the wrong file can then see which one it was.
// This flavour has a byte below the printable range and one above.
TEST(FontTest, RefusesAnUnknownFlavour)
{
    EXPECT_THROW(
        (void)Font{directoryWithOneTable(0x12348078)}, TtfError);
}

TEST(FontTest, RefusesAFontDeclaringNoTables)
{
    const std::array<TableRecord, 0> records{};

    EXPECT_THROW(
        (void)Font{buildDirectory(0x00010000, 0, records, kRoom)},
        TtfError);
}

TEST(FontTest, RefusesADirectoryRunningPastTheEnd)
{
    const std::array<TableRecord, 0> records{};

    EXPECT_THROW(
        (void)Font{buildDirectory(0x00010000, 100, records, kRoom)},
        TtfError);
}

TEST(FontTest, RefusesATableStartingPastTheEnd)
{
    const std::array<TableRecord, 1> records{
        TableRecord{.tag = "head", .offset = 100000, .length = 0}};

    EXPECT_THROW(
        (void)Font{buildDirectory(0x00010000, 1, records, kRoom)},
        TtfError);
}

TEST(FontTest, RefusesATableEndingPastTheEnd)
{
    const std::array<TableRecord, 1> records{
        TableRecord{.tag = "head", .offset = 28, .length = 100000}};

    EXPECT_THROW(
        (void)Font{buildDirectory(0x00010000, 1, records, kRoom)},
        TtfError);
}

// A directory that checks out, and a font still not drawable from.
// The rasteriser decides that, by refusing to open the font at all.
TEST(FontTest, RefusesAFontWithoutTheTablesToDrawFrom)
{
    EXPECT_THROW(
        (void)Font{directoryWithOneTable(0x00010000)}, TtfError);
}

TEST(FontTest, Metrics_AreWholePixelsAtTheRequestedHeight)
{
    const Font font{buildFont()};

    EXPECT_EQ(
        font.metrics(kHeight),
        (FontMetrics{
            .ascent = 16,
            .descent = -4,
            .lineGap = 2,
            .lineHeight = 22}));
}

TEST(FontTest, Metrics_RefuseAPixelHeightOfZero)
{
    const Font font{buildFont()};

    EXPECT_THROW((void)font.metrics(0), TtfError);
}

TEST(FontTest, Has_TrueForACodepointTheFontDraws)
{
    const Font font{buildFont()};

    EXPECT_TRUE(font.has(U'A'));
    EXPECT_TRUE(font.has(U' '));
}

TEST(FontTest, Has_FalseForACodepointItFallsBackOn)
{
    const Font font{buildFont()};

    EXPECT_FALSE(font.has(U'Z'));
    EXPECT_FALSE(font.has(U'€'));
}

TEST(FontTest, GlyphMetrics_PlaceAGlyphAgainstThePen)
{
    const Font font{buildFont()};

    EXPECT_EQ(
        font.glyphMetrics(U'A', kHeight),
        (GlyphMetrics{.advance = 18, .bearingX = 2, .bearingY = -15}));
    EXPECT_EQ(
        font.glyphMetrics(U'B', kHeight),
        (GlyphMetrics{.advance = 10, .bearingX = 0, .bearingY = -9}));
}

TEST(FontTest, GlyphMetrics_RefuseAPixelHeightOfZero)
{
    const Font font{buildFont()};

    EXPECT_THROW((void)font.glyphMetrics(U'A', 0), TtfError);
}

TEST(FontTest, Kerning_IsTheAdjustmentTheFontHolds)
{
    const Font font{buildFont()};

    EXPECT_EQ(font.kerning(U'A', U'B', kHeight), -2);
    EXPECT_EQ(font.kerning(U'B', U'A', kHeight), 0);
}

TEST(FontTest, Kerning_IsZeroWithoutAKernTable)
{
    const Font font{buildFont(FontRecipe{.kerning = false})};

    EXPECT_EQ(font.kerning(U'A', U'B', kHeight), 0);
}

TEST(FontTest, Kerning_RefusesAPixelHeightOfZero)
{
    const Font font{buildFont()};

    EXPECT_THROW((void)font.kerning(U'A', U'B', 0), TtfError);
}

TEST(FontTest, Rasterise_DrawsTheGlyphIntoACompleteMask)
{
    const Font font{buildFont()};
    const Glyph glyph = font.rasterise(U'A', kHeight);

    EXPECT_EQ(
        glyph.metrics,
        (GlyphMetrics{.advance = 18, .bearingX = 2, .bearingY = -15}));
    EXPECT_EQ(glyph.coverage.width, 16u);
    EXPECT_EQ(glyph.coverage.height, 15u);
    EXPECT_TRUE(glyph.coverage.isComplete());
    EXPECT_EQ(glyph.coverage.at(8, 7), 255);
}

// A space is not a failure and not a special case.
// It is a glyph with an advance and nothing drawn.
TEST(FontTest, Rasterise_OfASpaceIsEmptyAndStillMovesThePen)
{
    const Font font{buildFont()};
    const Glyph glyph = font.rasterise(U' ', kHeight);

    EXPECT_EQ(glyph.metrics.advance, 8);
    EXPECT_EQ(glyph.coverage, (antwika::ttf::Coverage{}));
}

// Lookup is total, following antwika::i18n.
// Something drawing a frame gets an answer rather than an exception.
TEST(FontTest, Rasterise_OfAnUnmappedCodepointFallsBackToNotdef)
{
    const Font font{buildFont()};

    EXPECT_NO_THROW((void)font.rasterise(U'Z', kHeight));
    EXPECT_EQ(font.rasterise(U'Z', kHeight).metrics.advance, 10);
}

TEST(FontTest, Rasterise_RefusesAPixelHeightOfZero)
{
    const Font font{buildFont()};

    EXPECT_THROW((void)font.rasterise(U'A', 0), TtfError);
}
