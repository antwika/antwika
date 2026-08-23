#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include <antwika/font/Coverage.hpp>
#include <antwika/font/FontMetrics.hpp>
#include <antwika/font/GlyphAtlas.hpp>
#include <antwika/font/Rect.hpp>
#include <antwika/gfx/RectF.hpp>

#include "antwika/text/AtlasText.hpp"
#include "antwika/gfx/Color.hpp"
#include "antwika/text/GlyphBlit.hpp"
#include "antwika/gfx/Point.hpp"
#include "antwika/gfx/Size.hpp"
#include "antwika/gfx/mocks/MockRenderer.hpp"
#include "antwika/gfx/mocks/MockTexture.hpp"

using antwika::font::AtlasGlyph;
using antwika::font::Coverage;
using antwika::font::FontMetrics;
using antwika::font::GlyphAtlas;
using antwika::text::atlasTextBlits;
using antwika::text::atlasTextSize;
using antwika::gfx::Color;
using antwika::text::drawAtlasText;
using antwika::text::GlyphBlit;
using antwika::gfx::Point;
using antwika::gfx::Size;
using antwika::gfx::RectF;
using antwika::gfx::mocks::MockRenderer;
using antwika::gfx::mocks::MockTexture;
using testing::_;

namespace
{
    [[nodiscard]] GlyphAtlas testAtlas()
    {
        GlyphAtlas atlas;
        atlas.metrics = FontMetrics{
            .ascent = 8, .descent = -2, .lineGap = 1, .lineHeight = 11};
        atlas.coverage = Coverage{
            .width = 4,
            .height = 4,
            .samples = std::vector<std::uint8_t>(16, 0)};

        atlas.glyphs = {
            AtlasGlyph{
                .codepoint = U' ',
                .sourceRect = {},
                .metrics = {.advance = 3}},
            AtlasGlyph{
                .codepoint = U'A',
                .sourceRect = {.x = 0, .y = 0, .width = 2, .height = 3},
                .metrics
                = {.advance = 5, .bearingX = 1, .bearingY = -7}},
            AtlasGlyph{
                .codepoint = U'B',
                .sourceRect = {.x = 3, .y = 0, .width = 1, .height = 2},
                .metrics
                = {.advance = 4, .bearingX = 0, .bearingY = -6}}};

        return atlas;
    }

    constexpr Point kOriginPoint{.x = 10, .y = 20};

    constexpr Color kTintColor{
        .red = 200, .green = 100, .blue = 50, .alpha = 255};

    constexpr GlyphBlit kBlitForA{
        .sourceRect
        = {.originPoint = {.x = 0, .y = 0},
           .size = {.width = 2, .height = 3}},
        .destinationRect = {
            .originPoint = {.x = 11, .y = 21},
            .size = {.width = 2, .height = 3}}};

    constexpr GlyphBlit kBlitForB{
        .sourceRect
        = {.originPoint = {.x = 3, .y = 0},
           .size = {.width = 1, .height = 2}},
        .destinationRect = {
            .originPoint = {.x = 15, .y = 22},
            .size = {.width = 1, .height = 2}}};
}

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
        (Size{.width = 9, .height = 11}));
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
        atlasTextBlits(testAtlas(), kOriginPoint, "AB"),
        (std::vector<GlyphBlit>{kBlitForA, kBlitForB}));
}

TEST(AtlasTextTest, AtlasTextBlits_IsEmptyForEmptyText)
{
    EXPECT_TRUE(atlasTextBlits(testAtlas(), kOriginPoint, "").empty());
}

TEST(AtlasTextTest, AtlasTextBlits_DrawsNoSpaceAndStillMovesThePen)
{
    const std::vector<GlyphBlit> blits
        = atlasTextBlits(testAtlas(), kOriginPoint, " B");

    ASSERT_EQ(blits.size(), 1U);
    EXPECT_EQ(
        blits.front().destinationRect.originPoint,
        (Point{.x = kOriginPoint.x + 3, .y = 22}));
}

TEST(AtlasTextTest, AtlasTextBlits_MovesThePenNowhereForAnAbsentGlyph)
{
    const std::vector<GlyphBlit> blits
        = atlasTextBlits(testAtlas(), kOriginPoint, "zB");

    ASSERT_EQ(blits.size(), 1U);
    EXPECT_EQ(
        blits.front().destinationRect.originPoint,
        (Point{.x = kOriginPoint.x, .y = 22}));
}

TEST(AtlasTextTest, AtlasTextBlits_SkipsAGlyphReachingOutsideTheMask)
{
    GlyphAtlas atlas = testAtlas();
    atlas.glyphs[1].sourceRect.width = 99;

    EXPECT_TRUE(atlasTextBlits(atlas, kOriginPoint, "A").empty());
}

TEST(AtlasTextTest, AtlasTextBlits_ReadsAHighByteAsALatin1Codepoint)
{
    GlyphAtlas atlas = testAtlas();
    atlas.glyphs.push_back(AtlasGlyph{
        .codepoint = 0xc4,
        .sourceRect = {.x = 0, .y = 3, .width = 2, .height = 1},
        .metrics = {.advance = 5, .bearingX = 0, .bearingY = -9}});

    EXPECT_EQ(atlasTextBlits(atlas, kOriginPoint, "\xc4").size(), 1U);
}
TEST(AtlasTextTest, DrawAtlasText_BlitsOncePerDrawableCharacter)
{
    MockRenderer renderer;
    MockTexture texture;

    EXPECT_CALL(
        renderer,
        drawTexture(
            testing::Ref(texture),
            RectF{kBlitForA.sourceRect},
            RectF{kBlitForA.destinationRect},
            kTintColor));
    EXPECT_CALL(
        renderer,
        drawTexture(
            testing::Ref(texture),
            RectF{kBlitForB.sourceRect},
            RectF{kBlitForB.destinationRect},
            kTintColor));

    drawAtlasText(
        renderer, texture, testAtlas(), kOriginPoint, "AB", kTintColor);
}

TEST(AtlasTextTest, DrawAtlasText_DrawsNothingWhenNothingIsDrawable)
{
    MockRenderer renderer;
    MockTexture texture;

    EXPECT_CALL(renderer, drawTexture(_, _, _, _)).Times(0);

    drawAtlasText(
        renderer, texture, testAtlas(), kOriginPoint, "zz", kTintColor);
}
