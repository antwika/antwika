#include <gtest/gtest.h>

#include "antwika/gfx/Glyphs.hpp"

using antwika::gfx::getScaledGlyphAdvance;
using antwika::gfx::getScaledGlyphLineHeight;
using antwika::gfx::glyphAdvanceOf;
using antwika::gfx::glyphLineHeightOf;
using antwika::gfx::kGlyphAdvance;
using antwika::gfx::kGlyphLineHeight;
using antwika::gfx::kSmallGlyphAdvance;
using antwika::gfx::kSmallGlyphLineHeight;
using antwika::gfx::TextFace;
using antwika::gfx::TextScale;

TEST(GlyphMetricsTest, GlyphAdvanceOf_NarrowsForTheSmallFace)
{
    EXPECT_EQ(glyphAdvanceOf(TextFace::Normal), kGlyphAdvance);
    EXPECT_EQ(glyphAdvanceOf(TextFace::Small), kSmallGlyphAdvance);
}

TEST(GlyphMetricsTest, GlyphLineHeightOf_ShortensForTheSmallFace)
{
    EXPECT_EQ(glyphLineHeightOf(TextFace::Normal), kGlyphLineHeight);
    EXPECT_EQ(
        glyphLineHeightOf(TextFace::Small), kSmallGlyphLineHeight);
}

TEST(GlyphMetricsTest, ScaledGlyphAdvance_MultipliesTheFaceAdvance)
{
    const TextScale scale{.face = TextFace::Small, .multiplier = 3};

    EXPECT_EQ(getScaledGlyphAdvance(scale), kSmallGlyphAdvance * 3);
    EXPECT_EQ(
        getScaledGlyphAdvance(TextScale{.multiplier = 2}),
        kGlyphAdvance * 2);
}

TEST(GlyphMetricsTest, ScaledGlyphLineHeight_MultipliesTheFaceLineHeight)
{
    const TextScale scale{.face = TextFace::Small, .multiplier = 3};

    EXPECT_EQ(getScaledGlyphLineHeight(scale), kSmallGlyphLineHeight * 3);
    EXPECT_EQ(
        getScaledGlyphLineHeight(TextScale{.multiplier = 2}),
        kGlyphLineHeight * 2);
}
