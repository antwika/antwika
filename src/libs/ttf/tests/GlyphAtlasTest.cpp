#include "antwika/ttf/GlyphAtlas.hpp"

#include <array>
#include <cstdint>

#include <gtest/gtest.h>

#include "antwika/ttf/Font.hpp"
#include "antwika/ttf/GlyphMetrics.hpp"
#include "antwika/ttf/Rect.hpp"
#include "antwika/ttf/TtfError.hpp"

#include "SyntheticFont.hpp"

using antwika::ttf::AtlasGlyph;
using antwika::ttf::Font;
using antwika::ttf::GlyphAtlas;
using antwika::ttf::GlyphMetrics;
using antwika::ttf::makeGlyphAtlas;
using antwika::ttf::Rect;
using antwika::ttf::TtfError;
using antwika::ttf::tests::buildFont;

namespace
{
    constexpr std::uint32_t kHeight = 20;

    constexpr std::array<char32_t, 3> kAlphabet{U' ', U'A', U'B'};
} // namespace

TEST(GlyphAtlasTest, PacksEveryGlyphIntoOneMask)
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

// What was rasterised is what landed at the rectangle named.
// That is the whole contract a renderer blits on.
TEST(GlyphAtlasTest, BlitsTheGlyphsIntoTheirOwnRectangles)
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

TEST(GlyphAtlasTest, CarriesTheFontsOwnLineMetrics)
{
    const Font font{buildFont()};
    const GlyphAtlas atlas
        = makeGlyphAtlas(font, kAlphabet, kHeight);

    EXPECT_EQ(atlas.metrics, font.metrics(kHeight));
}

// A glyph with nothing drawn takes no room and keeps its advance.
// So laying a string out never has to ask which kind it got.
TEST(GlyphAtlasTest, GivesABlankGlyphNoRectangleAndItsMetrics)
{
    const Font font{buildFont()};
    const GlyphAtlas atlas
        = makeGlyphAtlas(font, kAlphabet, kHeight);

    EXPECT_EQ(atlas.glyphs[0].source, Rect{});
    EXPECT_EQ(atlas.glyphs[0].metrics.advance, 8);
}

TEST(GlyphAtlasTest, AnAtlasOfOnlyBlankGlyphsHasNoMaskAtAll)
{
    const Font font{buildFont()};
    const std::array<char32_t, 1> blank{U' '};
    const GlyphAtlas atlas = makeGlyphAtlas(font, blank, kHeight);

    EXPECT_EQ(atlas.coverage.width, 0u);
    EXPECT_EQ(atlas.coverage.height, 0u);
    EXPECT_TRUE(atlas.coverage.samples.empty());
}

TEST(GlyphAtlasTest, StartsANewShelfWhenARowIsFull)
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

// Duplicates collapse and the order asked for does not matter.
// That is what makes an atlas a function of the characters in it.
TEST(GlyphAtlasTest, IgnoresOrderAndRepetition)
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

TEST(GlyphAtlasTest, ComparesMaskMetricsAndEntries)
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

TEST(GlyphAtlasTest, RefusesAnAtlasOfNoCharacters)
{
    const Font font{buildFont()};
    const std::array<char32_t, 0> nothing{};

    EXPECT_THROW(
        (void)makeGlyphAtlas(font, nothing, kHeight), TtfError);
}

TEST(GlyphAtlasTest, RefusesAPixelHeightOfZero)
{
    const Font font{buildFont()};

    EXPECT_THROW((void)makeGlyphAtlas(font, kAlphabet, 0), TtfError);
}

// Wrapping cannot help a glyph wider than a whole row.
// So it is refused rather than packed somewhere it does not fit.
TEST(GlyphAtlasTest, RefusesAGlyphWiderThanTheAtlas)
{
    const Font font{buildFont()};

    EXPECT_THROW(
        (void)makeGlyphAtlas(
            font,
            kAlphabet,
            kHeight,
            GlyphAtlas::Options{.maxWidth = 10}),
        TtfError);
}
