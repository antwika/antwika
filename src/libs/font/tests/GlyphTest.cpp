#include <gtest/gtest.h>

#include "antwika/font/Glyph.hpp"

using antwika::font::Glyph;

namespace
{
    Glyph getReference()
    {
        return Glyph{
            .metrics = {.advance = 18, .bearingX = 2, .bearingY = -15},
            .coverage = {
                .width = 2, .height = 1, .samples = {128, 255}}};
    }
}

TEST(GlyphTest, OperatorEquals_IsTrueForTheSameMetricsAndCoverage)
{
    EXPECT_EQ(getReference(), getReference());
}

TEST(GlyphTest, OperatorEquals_IsFalseWhenTheMetricsDiffer)
{
    Glyph otherGlyph = getReference();
    otherGlyph.metrics.advance = 19;

    EXPECT_NE(getReference(), otherGlyph);
}

TEST(GlyphTest, OperatorEquals_IsFalseWhenTheCoverageDiffers)
{
    Glyph otherGlyph = getReference();
    otherGlyph.coverage.samples[0] = 0;

    EXPECT_NE(getReference(), otherGlyph);
}
