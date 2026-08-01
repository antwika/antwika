#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include <antwika/font/Coverage.hpp>
#include <antwika/font/FontMetrics.hpp>
#include <antwika/font/GlyphAtlas.hpp>
#include <antwika/font/Rect.hpp>

#include "antwika/gfx/AtlasText.hpp"
#include "antwika/gfx/Color.hpp"
#include "antwika/gfx/GlyphBlit.hpp"
#include "antwika/gfx/Point.hpp"
#include "antwika/gfx/Size.hpp"
#include "antwika/gfx/mocks/MockRenderer.hpp"
#include "antwika/gfx/mocks/MockTexture.hpp"

using antwika::font::AtlasGlyph;
using antwika::font::Coverage;
using antwika::font::FontMetrics;
using antwika::font::GlyphAtlas;
using antwika::gfx::atlasTextBlits;
using antwika::gfx::atlasTextSize;
using antwika::gfx::Color;
using antwika::gfx::drawAtlasText;
using antwika::gfx::GlyphBlit;
using antwika::gfx::Point;
using antwika::gfx::Size;
using antwika::gfx::mocks::MockRenderer;
using antwika::gfx::mocks::MockTexture;
using testing::_;

namespace
{
    // Hand-built rather than rasterised from a font.
    // A GlyphAtlas is a plain value, so every number is stated here.
    // No test reads a file to get one.
    // SyntheticFont covers the parsing side, and this is not parsing.
    [[nodiscard]] GlyphAtlas testAtlas()
    {
        GlyphAtlas atlas;
        atlas.metrics = FontMetrics{
            .ascent = 8, .descent = -2, .lineGap = 1, .lineHeight = 11};
        atlas.coverage = Coverage{
            .width = 4,
            .height = 4,
            .samples = std::vector<std::uint8_t>(16, 0)};

        // Sorted by codepoint, as makeGlyphAtlas leaves them.
        // find() is a binary search over exactly that order.
        atlas.glyphs = {
            // A space keeps its advance and gets no rectangle.
            AtlasGlyph{
                .codepoint = U' ',
                .source = {},
                .metrics = {.advance = 3}},
            AtlasGlyph{
                .codepoint = U'A',
                .source = {.x = 0, .y = 0, .width = 2, .height = 3},
                .metrics
                = {.advance = 5, .bearingX = 1, .bearingY = -7}},
            AtlasGlyph{
                .codepoint = U'B',
                .source = {.x = 3, .y = 0, .width = 1, .height = 2},
                .metrics
                = {.advance = 4, .bearingX = 0, .bearingY = -6}}};

        return atlas;
    }

    constexpr Point kOrigin{.x = 10, .y = 20};

    constexpr Color kTint{
        .red = 200, .green = 100, .blue = 50, .alpha = 255};

    constexpr GlyphBlit kBlitForA{
        .source
        = {.origin = {.x = 0, .y = 0},
           .size = {.width = 2, .height = 3}},
        .destination = {
            .origin = {.x = 11, .y = 21},
            .size = {.width = 2, .height = 3}}};

    constexpr GlyphBlit kBlitForB{
        .source
        = {.origin = {.x = 3, .y = 0},
           .size = {.width = 1, .height = 2}},
        .destination = {
            .origin = {.x = 15, .y = 22},
            .size = {.width = 1, .height = 2}}};
} // namespace

TEST(AtlasTextTest, AtlasTextSize_SumsTheAdvancesOverOneLine)
{
    EXPECT_EQ(
        atlasTextSize(testAtlas(), "AB"),
        (Size{.width = 9, .height = 11}));
}

TEST(AtlasTextTest, AtlasTextSize_CountsASpaceLikeAnyOtherCharacter)
{
    EXPECT_EQ(
        atlasTextSize(testAtlas(), "A B"),
        (Size{.width = 12, .height = 11}));
}

TEST(AtlasTextTest, AtlasTextSize_IsZeroForEmptyText)
{
    EXPECT_EQ(atlasTextSize(testAtlas(), ""), Size{});
}

TEST(AtlasTextTest, AtlasTextSize_MeasuresNothingForAnAbsentCharacter)
{
    EXPECT_EQ(
        atlasTextSize(testAtlas(), "AzB"),
        atlasTextSize(testAtlas(), "AB"));
}

TEST(AtlasTextTest, AtlasTextSize_ReportsANegativeTotalAsZero)
{
    GlyphAtlas atlas = testAtlas();
    atlas.glyphs[1].metrics.advance = -100;

    EXPECT_EQ(
        atlasTextSize(atlas, "AB"), (Size{.width = 0, .height = 11}));
}

TEST(AtlasTextTest, AtlasTextBlits_WalksThePenAlongTheBaseline)
{
    EXPECT_EQ(
        atlasTextBlits(testAtlas(), kOrigin, "AB"),
        (std::vector<GlyphBlit>{kBlitForA, kBlitForB}));
}

TEST(AtlasTextTest, AtlasTextBlits_IsEmptyForEmptyText)
{
    EXPECT_TRUE(atlasTextBlits(testAtlas(), kOrigin, "").empty());
}

TEST(AtlasTextTest, AtlasTextBlits_DrawsNoSpaceAndStillMovesThePen)
{
    const std::vector<GlyphBlit> blits
        = atlasTextBlits(testAtlas(), kOrigin, " B");

    ASSERT_EQ(blits.size(), 1U);
    EXPECT_EQ(
        blits.front().destination.origin,
        (Point{.x = kOrigin.x + 3, .y = 22}));
}

TEST(AtlasTextTest, AtlasTextBlits_MovesThePenNowhereForAnAbsentGlyph)
{
    const std::vector<GlyphBlit> blits
        = atlasTextBlits(testAtlas(), kOrigin, "zB");

    ASSERT_EQ(blits.size(), 1U);
    EXPECT_EQ(
        blits.front().destination.origin,
        (Point{.x = kOrigin.x, .y = 22}));
}

TEST(AtlasTextTest, AtlasTextBlits_SkipsAGlyphReachingOutsideTheMask)
{
    GlyphAtlas atlas = testAtlas();
    atlas.glyphs[1].source.width = 99;

    EXPECT_TRUE(atlasTextBlits(atlas, kOrigin, "A").empty());
}

TEST(AtlasTextTest, AtlasTextBlits_ReadsAHighByteAsALatin1Codepoint)
{
    GlyphAtlas atlas = testAtlas();
    atlas.glyphs.push_back(AtlasGlyph{
        .codepoint = 0xc4,
        .source = {.x = 0, .y = 3, .width = 2, .height = 1},
        .metrics = {.advance = 5, .bearingX = 0, .bearingY = -9}});

    EXPECT_EQ(atlasTextBlits(atlas, kOrigin, "\xc4").size(), 1U);
}

TEST(AtlasTextTest, AtlasTextBlits_IsThePureFunctionItLooksLike)
{
    EXPECT_EQ(
        atlasTextBlits(testAtlas(), kOrigin, "A B"),
        atlasTextBlits(testAtlas(), kOrigin, "A B"));
}

TEST(AtlasTextTest, DrawAtlasText_BlitsOncePerDrawableCharacter)
{
    MockRenderer renderer;
    MockTexture texture;

    EXPECT_CALL(
        renderer,
        drawTexture(
            testing::Ref(texture),
            kBlitForA.source,
            kBlitForA.destination,
            kTint));
    EXPECT_CALL(
        renderer,
        drawTexture(
            testing::Ref(texture),
            kBlitForB.source,
            kBlitForB.destination,
            kTint));

    drawAtlasText(
        renderer, texture, testAtlas(), kOrigin, "AB", kTint);
}

TEST(AtlasTextTest, DrawAtlasText_DrawsNothingWhenNothingIsDrawable)
{
    MockRenderer renderer;
    MockTexture texture;

    EXPECT_CALL(renderer, drawTexture(_, _, _, _)).Times(0);

    drawAtlasText(
        renderer, texture, testAtlas(), kOrigin, "zz", kTint);
}
