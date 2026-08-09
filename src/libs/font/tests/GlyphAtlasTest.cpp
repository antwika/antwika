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
using antwika::font::makeGlyphAtlas;
using antwika::font::Rect;
using antwika::font::FontError;
using antwika::font::tests::buildFont;

namespace
{
    constexpr std::uint32_t kHeight = 20;

    constexpr std::array<char32_t, 3> kAlphabet{U' ', U'A', U'B'};
}

TEST(GlyphAtlasTest, MakeGlyphAtlas_PacksEveryGlyphIntoOneMask)
{
    const Font font{buildFont()};
    const GlyphAtlas atlas
        = makeGlyphAtlas(font, kAlphabet, kHeight);

    ASSERT_EQ(atlas.glyphs.size(), 3u);
    EXPECT_EQ(atlas.coverage.width, 28u);
    EXPECT_EQ(atlas.coverage.height, 17u);
    EXPECT_TRUE(atlas.coverage.isComplete());
    EXPECT_EQ(
        atlas.glyphs[1].source, (Rect{.x = 1, .y = 1, .width = 16,
            .height = 15}));
    EXPECT_EQ(
        atlas.glyphs[2].source, (Rect{.x = 18, .y = 1, .width = 9,
            .height = 9}));
}

TEST(GlyphAtlasTest, MakeGlyphAtlas_BlitsGlyphsIntoOwnRectangles)
{
    const Font font{buildFont()};
    const GlyphAtlas atlas
        = makeGlyphAtlas(font, kAlphabet, kHeight);

    EXPECT_EQ(atlas.coverage.at(1 + 8, 1 + 7),
        font.rasterise(U'A', kHeight).coverage.at(8, 7));
    EXPECT_EQ(atlas.coverage.at(18 + 4, 1 + 4),
        font.rasterise(U'B', kHeight).coverage.at(4, 4));
    EXPECT_EQ(atlas.coverage.at(0, 0), 0);
}

TEST(GlyphAtlasTest, MakeGlyphAtlas_BlitsTheTopRowOfAGlyphToo)
{
    const Font font{buildFont()};
    const GlyphAtlas atlas
        = makeGlyphAtlas(font, kAlphabet, kHeight);
    const antwika::font::Coverage mask
        = font.rasterise(U'A', kHeight).coverage;

    std::vector<std::uint8_t> drawn;
    std::vector<std::uint8_t> packed;

    for (std::uint32_t x = 0; x < mask.width; ++x)
    {
        drawn.push_back(mask.at(x, 0));
        packed.push_back(atlas.coverage.at(1 + x, 1));
    }

    ASSERT_NE(drawn, std::vector<std::uint8_t>(mask.width, 0));
    EXPECT_EQ(packed, drawn);
}

TEST(GlyphAtlasTest, AtlasGlyph_StartsOnNoCharacterAtAll)
{
    constexpr AtlasGlyph fresh;

    EXPECT_EQ(fresh.codepoint, U'\0');
}

TEST(GlyphAtlasTest, MakeGlyphAtlas_CarriesTheFontsLineMetrics)
{
    const Font font{buildFont()};
    const GlyphAtlas atlas
        = makeGlyphAtlas(font, kAlphabet, kHeight);

    EXPECT_EQ(atlas.metrics, font.metrics(kHeight));
}

TEST(GlyphAtlasTest, MakeGlyphAtlas_GivesABlankGlyphNoRectangle)
{
    const Font font{buildFont()};
    const GlyphAtlas atlas
        = makeGlyphAtlas(font, kAlphabet, kHeight);

    EXPECT_EQ(atlas.glyphs[0].source, Rect{});
    EXPECT_EQ(atlas.glyphs[0].metrics.advance, 8);
}

TEST(GlyphAtlasTest, MakeGlyphAtlas_MakesNoMaskForOnlyBlanks)
{
    const Font font{buildFont()};
    const std::array<char32_t, 1> blank{U' '};
    const GlyphAtlas atlas = makeGlyphAtlas(font, blank, kHeight);

    EXPECT_EQ(atlas.coverage.width, 0u);
    EXPECT_EQ(atlas.coverage.height, 0u);
    EXPECT_TRUE(atlas.coverage.samples.empty());
}

TEST(GlyphAtlasTest, MakeGlyphAtlas_StartsANewShelfWhenARowIsFull)
{
    const Font font{buildFont()};
    const GlyphAtlas atlas = makeGlyphAtlas(
        font, kAlphabet, kHeight, GlyphAtlas::Options{.maxWidth = 20});

    EXPECT_EQ(
        atlas.glyphs[1].source, (Rect{.x = 1, .y = 1, .width = 16,
            .height = 15}));
    EXPECT_EQ(
        atlas.glyphs[2].source, (Rect{.x = 1, .y = 17, .width = 9,
            .height = 9}));
    EXPECT_EQ(atlas.coverage.width, 18u);
    EXPECT_EQ(atlas.coverage.height, 27u);
}

TEST(GlyphAtlasTest, MakeGlyphAtlas_IgnoresOrderAndRepetition)
{
    const Font font{buildFont()};
    const std::array<char32_t, 5> jumbled{
        U'B', U'A', U'B', U' ', U'A'};

    EXPECT_EQ(
        makeGlyphAtlas(font, jumbled, kHeight),
        makeGlyphAtlas(font, kAlphabet, kHeight));
}

TEST(GlyphAtlasTest, Find_AnswersForACharacterItHolds)
{
    const Font font{buildFont()};
    const GlyphAtlas atlas
        = makeGlyphAtlas(font, kAlphabet, kHeight);
    const AtlasGlyph *found = atlas.find(U'A');

    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->codepoint, U'A');
    EXPECT_EQ(
        found->metrics,
        (GlyphMetrics{.advance = 18, .bearingX = 2, .bearingY = -15}));
}

TEST(GlyphAtlasTest, Find_AnswersNothingForACharacterPastTheEnd)
{
    const Font font{buildFont()};
    const GlyphAtlas atlas
        = makeGlyphAtlas(font, kAlphabet, kHeight);

    EXPECT_EQ(atlas.find(U'Z'), nullptr);
}

TEST(GlyphAtlasTest, Find_AnswersNothingForAGapInTheMiddle)
{
    const Font font{buildFont()};
    const GlyphAtlas atlas
        = makeGlyphAtlas(font, kAlphabet, kHeight);

    EXPECT_EQ(atlas.find(U'0'), nullptr);
}

TEST(GlyphAtlasTest, AtlasGlyph_ComparesCodepointRectangleAndMetrics)
{
    constexpr AtlasGlyph reference{
        .codepoint = U'A',
        .source = {.x = 1, .y = 2, .width = 3, .height = 4},
        .metrics = {.advance = 5, .bearingX = 6, .bearingY = 7}};

    AtlasGlyph elsewhere = reference;
    elsewhere.source.x = 9;

    AtlasGlyph wider = reference;
    wider.metrics.advance = 9;

    AtlasGlyph other = reference;
    other.codepoint = U'B';

    EXPECT_EQ(reference, AtlasGlyph{reference});
    EXPECT_NE(reference, other);
    EXPECT_NE(reference, elsewhere);
    EXPECT_NE(reference, wider);
}

TEST(GlyphAtlasTest, OperatorEquals_ComparesMaskMetricsAndEntries)
{
    const Font font{buildFont()};
    const GlyphAtlas atlas
        = makeGlyphAtlas(font, kAlphabet, kHeight);

    GlyphAtlas repainted = atlas;
    repainted.coverage.samples[0] = 7;

    GlyphAtlas remeasured = atlas;
    remeasured.metrics.ascent = 99;

    GlyphAtlas shortened = atlas;
    shortened.glyphs.pop_back();

    EXPECT_EQ(atlas, makeGlyphAtlas(font, kAlphabet, kHeight));
    EXPECT_NE(atlas, repainted);
    EXPECT_NE(atlas, remeasured);
    EXPECT_NE(atlas, shortened);
}

TEST(GlyphAtlasTest, MakeGlyphAtlas_RefusesNoCharacters)
{
    const Font font{buildFont()};
    const std::array<char32_t, 0> nothing{};

    EXPECT_THROW(
        (void)makeGlyphAtlas(font, nothing, kHeight), FontError);
}

TEST(GlyphAtlasTest, MakeGlyphAtlas_RefusesAPixelHeightOfZero)
{
    const Font font{buildFont()};

    EXPECT_THROW((void)makeGlyphAtlas(font, kAlphabet, 0), FontError);
}

TEST(GlyphAtlasTest, MakeGlyphAtlas_RefusesAGlyphWiderThanTheAtlas)
{
    const Font font{buildFont()};

    EXPECT_THROW(
        (void)makeGlyphAtlas(
            font,
            kAlphabet,
            kHeight,
            GlyphAtlas::Options{.maxWidth = 10}),
        FontError);
}
