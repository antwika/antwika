#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "antwika/font/Font.hpp"
#include "antwika/font/FontMetrics.hpp"
#include "antwika/font/Glyph.hpp"
#include "antwika/font/GlyphMetrics.hpp"
#include "antwika/font/FontError.hpp"
#include "SyntheticFont.hpp"

using antwika::font::Font;
using antwika::font::FontMetrics;
using antwika::font::Glyph;
using antwika::font::GlyphMetrics;
using antwika::font::FontError;
using antwika::font::tests::buildDirectory;
using antwika::font::tests::buildFont;
using antwika::font::tests::FontRecipe;
using antwika::font::tests::TableRecord;

namespace
{
    constexpr std::uint32_t kHeight = 20;

    constexpr std::size_t kRoom = 64;

    std::vector<std::uint8_t> directoryWithOneTable(
        std::uint32_t flavour)
    {
        const std::array<TableRecord, 1> records{
            TableRecord{.tag = "head", .offset = 28, .length = 4}};

        return buildDirectory(flavour, 1, records, kRoom);
    }

    [[nodiscard]] std::string refusalOf(std::vector<std::uint8_t> bytes)
    {
        try
        {
            (void)Font{std::move(bytes)};
        }
        catch (const FontError &refused)
        {
            return refused.what();
        }

        return "these bytes were read as a font";
    }
}

TEST(FontTest, BuildFont_ReadsAWholeFont)
{
    EXPECT_NO_THROW((void)Font{buildFont()});
}

TEST(FontTest, BuildFont_ReadsTheAppleFlavour)
{
    EXPECT_NO_THROW(
        (void)Font{buildFont(FontRecipe{.flavour = 0x74727565})});
}

TEST(FontTest, BuildFont_RefusesBytesTooShortForAnOffsetTable)
{
    EXPECT_THROW((void)Font{std::vector<std::uint8_t>(4, 0)}, FontError);
}

TEST(FontTest, BuildFont_RefusesAFontCollection)
{
    EXPECT_THROW(
        (void)Font{directoryWithOneTable(0x74746366)}, FontError);
}

TEST(FontTest, BuildFont_RefusesCffOutlines)
{
    EXPECT_THROW(
        (void)Font{directoryWithOneTable(0x4F54544F)}, FontError);
}

TEST(FontTest, BuildFont_RefusesAnUnknownFlavour)
{
    EXPECT_THROW(
        (void)Font{directoryWithOneTable(0x12348078)}, FontError);
}

TEST(FontTest, BuildFont_QuotesTheFourCharactersItFoundInstead)
{
    EXPECT_THAT(
        refusalOf(directoryWithOneTable(0x12348078)),
        testing::HasSubstr("open with '?4?x' rather than"));
}

TEST(FontTest, BuildFont_TakesATableStartingWhereTheFontEnds)
{
    const std::array<TableRecord, 1> records{
        TableRecord{.tag = "head", .offset = kRoom, .length = 0}};

    EXPECT_THAT(
        refusalOf(buildDirectory(0x00010000, 1, records, kRoom)),
        testing::HasSubstr("do not carry the tables"));
}

TEST(FontTest, BuildFont_RefusesAFontDeclaringNoTables)
{
    const std::array<TableRecord, 0> records{};

    EXPECT_THROW(
        (void)Font{buildDirectory(0x00010000, 0, records, kRoom)},
        FontError);
}

TEST(FontTest, BuildFont_RefusesADirectoryPastTheEnd)
{
    const std::array<TableRecord, 0> records{};

    EXPECT_THROW(
        (void)Font{buildDirectory(0x00010000, 100, records, kRoom)},
        FontError);
}

TEST(FontTest, BuildFont_RefusesATableStartingPastTheEnd)
{
    const std::array<TableRecord, 1> records{
        TableRecord{.tag = "head", .offset = 100000, .length = 0}};

    EXPECT_THROW(
        (void)Font{buildDirectory(0x00010000, 1, records, kRoom)},
        FontError);
}

TEST(FontTest, BuildFont_RefusesATableEndingPastTheEnd)
{
    const std::array<TableRecord, 1> records{
        TableRecord{.tag = "head", .offset = 28, .length = 100000}};

    EXPECT_THROW(
        (void)Font{buildDirectory(0x00010000, 1, records, kRoom)},
        FontError);
}

TEST(FontTest, BuildFont_RefusesAFontMissingDrawingTables)
{
    EXPECT_THROW(
        (void)Font{directoryWithOneTable(0x00010000)}, FontError);
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

    EXPECT_THROW((void)font.metrics(0), FontError);
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

    EXPECT_THROW((void)font.glyphMetrics(U'A', 0), FontError);
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

TEST(FontTest, Rasterise_OfASpaceIsEmptyAndStillMovesThePen)
{
    const Font font{buildFont()};
    const Glyph glyph = font.rasterise(U' ', kHeight);

    EXPECT_EQ(glyph.metrics.advance, 8);
    EXPECT_EQ(glyph.coverage, (antwika::font::Coverage{}));
}

TEST(FontTest, Rasterise_OfAnUnmappedCodepointFallsBackToNotdef)
{
    const Font font{buildFont()};

    EXPECT_NO_THROW((void)font.rasterise(U'Z', kHeight));
    EXPECT_EQ(font.rasterise(U'Z', kHeight).metrics.advance, 10);
}

TEST(FontTest, Rasterise_RefusesAPixelHeightOfZero)
{
    const Font font{buildFont()};

    EXPECT_THROW((void)font.rasterise(U'A', 0), FontError);
}
