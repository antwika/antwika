#include <gtest/gtest.h>

#include <cstdint>

#include "antwika/gfx/Glyphs.hpp"

using antwika::gfx::getEncodeTextScale;
using antwika::gfx::glyphAdvanceOf;
using antwika::gfx::glyphLineHeightOf;
using antwika::gfx::kGlyphAdvance;
using antwika::gfx::kGlyphLineHeight;
using antwika::gfx::kSmallGlyphAdvance;
using antwika::gfx::kSmallGlyphLineHeight;
using antwika::gfx::TextFace;
using antwika::gfx::textFaceOf;
using antwika::gfx::textMultiplierOf;

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

TEST(GlyphMetricsTest, EncodeTextScale_RoundTripsThroughBothParts)
{
    const auto scale = getEncodeTextScale(TextFace::Small, 3);

    EXPECT_EQ(textFaceOf(scale), TextFace::Small);
    EXPECT_EQ(textMultiplierOf(scale), 3U);
}

TEST(GlyphMetricsTest, EncodeTextScale_LeavesANormalScaleBare)
{
    EXPECT_EQ(getEncodeTextScale(TextFace::Normal, 2), 2U);
    EXPECT_EQ(textFaceOf(2), TextFace::Normal);
}
