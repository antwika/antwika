#include <gtest/gtest.h>

#include "antwika/font/Glyph.hpp"

using antwika::font::Glyph;

namespace
{
    Glyph reference()
    {
        return Glyph{
            .metrics = {.advance = 18, .bearingX = 2, .bearingY = -15},
            .coverage = {
                .width = 2, .height = 1, .samples = {128, 255}}};
    }
}

TEST(GlyphTest, OperatorEquals_IsTrueForTheSameMetricsAndCoverage)
{
    EXPECT_EQ(reference(), reference());
}

TEST(GlyphTest, OperatorEquals_IsFalseWhenTheMetricsDiffer)
{
    Glyph otherGlyph = reference();
    otherGlyph.metrics.advance = 19;

    EXPECT_NE(reference(), otherGlyph);
}

TEST(GlyphTest, OperatorEquals_IsFalseWhenTheCoverageDiffers)
{
    Glyph otherGlyph = reference();
    otherGlyph.coverage.samples[0] = 0;

    EXPECT_NE(reference(), otherGlyph);
}
