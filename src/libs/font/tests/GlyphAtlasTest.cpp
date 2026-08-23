#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <vector>

#include "antwika/font/Coverage.hpp"
#include "antwika/font/GlyphAtlas.hpp"
#include "antwika/font/Font.hpp"
#include "antwika/font/GlyphMetrics.hpp"
#include "antwika/font/Rect.hpp"
#include "antwika/font/FontError.hpp"
#include "SyntheticFont.hpp"

using antwika::font::AtlasGlyph;
using antwika::font::Font;
using antwika::font::GlyphAtlas;
using antwika::font::GlyphMetrics;
using antwika::font::createGlyphAtlas;
using antwika::font::Rect;
using antwika::font::FontError;
using antwika::font::tests::createFont;

namespace
{
    constexpr std::uint32_t kHeight = 20;

    constexpr std::array<char32_t, 3> kAlphabet{U' ', U'A', U'B'};
}

TEST(GlyphAtlasTest, MakeGlyphAtlas_PacksEveryGlyphIntoOneMask)
{
    const Font font{createFont()};
    const GlyphAtlas atlas
        = createGlyphAtlas(font, kAlphabet, kHeight);

    ASSERT_EQ(atlas.glyphs.size(), 3u);
    EXPECT_EQ(atlas.coverage.width, 28u);
    EXPECT_EQ(atlas.coverage.height, 17u);
    EXPECT_TRUE(atlas.coverage.isValid());
    EXPECT_EQ(
        atlas.glyphs[1].sourceRect, (Rect{.x = 1, .y = 1, .width = 16,
            .height = 15}));
    EXPECT_EQ(
        atlas.glyphs[2].sourceRect, (Rect{.x = 18, .y = 1, .width = 9,
            .height = 9}));
}

TEST(GlyphAtlasTest, MakeGlyphAtlas_BlitsGlyphsIntoOwnRectangles)
{
    const Font font{createFont()};
    const GlyphAtlas atlas
        = createGlyphAtlas(font, kAlphabet, kHeight);

    EXPECT_EQ(atlas.coverage.getEntryAt(1 + 8, 1 + 7),
        font.getRasterise(U'A', kHeight).coverage.getEntryAt(8, 7));
    EXPECT_EQ(atlas.coverage.getEntryAt(18 + 4, 1 + 4),
        font.getRasterise(U'B', kHeight).coverage.getEntryAt(4, 4));
    EXPECT_EQ(atlas.coverage.getEntryAt(0, 0), 0);
}

TEST(GlyphAtlasTest, MakeGlyphAtlas_BlitsTheTopRowOfAGlyphToo)
{
    const Font font{createFont()};
    const GlyphAtlas atlas
        = createGlyphAtlas(font, kAlphabet, kHeight);
    const antwika::font::Coverage maskCoverage
        = font.getRasterise(U'A', kHeight).coverage;

    std::vector<std::uint8_t> drawnBytes;
    std::vector<std::uint8_t> packedBytes;

    for (std::uint32_t x = 0; x < maskCoverage.width; ++x)
    {
        drawnBytes.push_back(maskCoverage.getEntryAt(x, 0));
        packedBytes.push_back(atlas.coverage.getEntryAt(1 + x, 1));
    }

    ASSERT_NE(drawnBytes, std::vector<std::uint8_t>(maskCoverage.width, 0));
    EXPECT_EQ(packedBytes, drawnBytes);
}

TEST(GlyphAtlasTest, AtlasGlyph_StartsOnNoCharacterAtAll)
{
    constexpr AtlasGlyph freshGlyph;

    EXPECT_EQ(freshGlyph.codepoint, U'\0');
}

TEST(GlyphAtlasTest, MakeGlyphAtlas_CarriesTheFontsLineMetrics)
{
    const Font font{createFont()};
    const GlyphAtlas atlas
        = createGlyphAtlas(font, kAlphabet, kHeight);

    EXPECT_EQ(atlas.metrics, font.getMetrics(kHeight));
}

TEST(GlyphAtlasTest, MakeGlyphAtlas_GivesABlankGlyphNoRectangle)
{
    const Font font{createFont()};
    const GlyphAtlas atlas
        = createGlyphAtlas(font, kAlphabet, kHeight);

    EXPECT_EQ(atlas.glyphs[0].sourceRect, Rect{});
    EXPECT_EQ(atlas.glyphs[0].metrics.advance, 8);
}

TEST(GlyphAtlasTest, MakeGlyphAtlas_MakesNoMaskForOnlyBlanks)
{
    const Font font{createFont()};
    const std::array<char32_t, 1> blank{U' '};
    const GlyphAtlas atlas = createGlyphAtlas(font, blank, kHeight);

    EXPECT_EQ(atlas.coverage.width, 0u);
    EXPECT_EQ(atlas.coverage.height, 0u);
    EXPECT_TRUE(atlas.coverage.samples.empty());
}

TEST(GlyphAtlasTest, MakeGlyphAtlas_StartsANewShelfWhenARowIsFull)
{
    const Font font{createFont()};
    const GlyphAtlas atlas = createGlyphAtlas(
        font, kAlphabet, kHeight, GlyphAtlas::Options{.maxWidth = 20});

    EXPECT_EQ(
        atlas.glyphs[1].sourceRect, (Rect{.x = 1, .y = 1, .width = 16,
            .height = 15}));
    EXPECT_EQ(
        atlas.glyphs[2].sourceRect, (Rect{.x = 1, .y = 17, .width = 9,
            .height = 9}));
    EXPECT_EQ(atlas.coverage.width, 18u);
    EXPECT_EQ(atlas.coverage.height, 27u);
}

TEST(GlyphAtlasTest, MakeGlyphAtlas_IgnoresOrderAndRepetition)
{
    const Font font{createFont()};
    const std::array<char32_t, 5> jumbledCodepoints{
        U'B', U'A', U'B', U' ', U'A'};

    EXPECT_EQ(
        createGlyphAtlas(font, jumbledCodepoints, kHeight),
        createGlyphAtlas(font, kAlphabet, kHeight));
}

TEST(GlyphAtlasTest, Find_AnswersForACharacterItHolds)
{
    const Font font{createFont()};
    const GlyphAtlas atlas
        = createGlyphAtlas(font, kAlphabet, kHeight);
    const AtlasGlyph *foundGlyph = atlas.getFind(U'A');

    ASSERT_NE(foundGlyph, nullptr);
    EXPECT_EQ(foundGlyph->codepoint, U'A');
    EXPECT_EQ(
        foundGlyph->metrics,
        (GlyphMetrics{.advance = 18, .bearingX = 2, .bearingY = -15}));
}

TEST(GlyphAtlasTest, Find_AnswersNothingForACharacterPastTheEnd)
{
    const Font font{createFont()};
    const GlyphAtlas atlas
        = createGlyphAtlas(font, kAlphabet, kHeight);

    EXPECT_EQ(atlas.getFind(U'Z'), nullptr);
}

TEST(GlyphAtlasTest, Find_AnswersNothingForAGapInTheMiddle)
{
    const Font font{createFont()};
    const GlyphAtlas atlas
        = createGlyphAtlas(font, kAlphabet, kHeight);

    EXPECT_EQ(atlas.getFind(U'0'), nullptr);
}

TEST(GlyphAtlasTest, AtlasGlyph_ComparesCodepointRectangleAndMetrics)
{
    constexpr AtlasGlyph referenceGlyph{
        .codepoint = U'A',
        .sourceRect = {.x = 1, .y = 2, .width = 3, .height = 4},
        .metrics = {.advance = 5, .bearingX = 6, .bearingY = 7}};

    AtlasGlyph elsewhereGlyph = referenceGlyph;
    elsewhereGlyph.sourceRect.x = 9;

    AtlasGlyph widerGlyph = referenceGlyph;
    widerGlyph.metrics.advance = 9;

    AtlasGlyph otherGlyph = referenceGlyph;
    otherGlyph.codepoint = U'B';

    EXPECT_EQ(referenceGlyph, AtlasGlyph{referenceGlyph});
    EXPECT_NE(referenceGlyph, otherGlyph);
    EXPECT_NE(referenceGlyph, elsewhereGlyph);
    EXPECT_NE(referenceGlyph, widerGlyph);
}

TEST(GlyphAtlasTest, OperatorEquals_ComparesMaskMetricsAndEntries)
{
    const Font font{createFont()};
    const GlyphAtlas atlas
        = createGlyphAtlas(font, kAlphabet, kHeight);

    GlyphAtlas repaintedAtlas = atlas;
    repaintedAtlas.coverage.samples[0] = 7;

    GlyphAtlas remeasuredAtlas = atlas;
    remeasuredAtlas.metrics.ascent = 99;

    GlyphAtlas shortenedAtlas = atlas;
    shortenedAtlas.glyphs.pop_back();

    EXPECT_EQ(atlas, createGlyphAtlas(font, kAlphabet, kHeight));
    EXPECT_NE(atlas, repaintedAtlas);
    EXPECT_NE(atlas, remeasuredAtlas);
    EXPECT_NE(atlas, shortenedAtlas);
}

TEST(GlyphAtlasTest, MakeGlyphAtlas_RefusesNoCharacters)
{
    const Font font{createFont()};
    const std::array<char32_t, 0> nothing{};

    EXPECT_THROW(
        (void)createGlyphAtlas(font, nothing, kHeight), FontError);
}

TEST(GlyphAtlasTest, MakeGlyphAtlas_RefusesAPixelHeightOfZero)
{
    const Font font{createFont()};

    EXPECT_THROW((void)createGlyphAtlas(font, kAlphabet, 0), FontError);
}

TEST(GlyphAtlasTest, MakeGlyphAtlas_RefusesAGlyphWiderThanTheAtlas)
{
    const Font font{createFont()};

    EXPECT_THROW(
        (void)createGlyphAtlas(
            font,
            kAlphabet,
            kHeight,
            GlyphAtlas::Options{.maxWidth = 10}),
        FontError);
}
