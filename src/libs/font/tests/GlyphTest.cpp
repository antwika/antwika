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
} // namespace

TEST(GlyphTest, Equality_IsTrueForTheSameMetricsAndCoverage)
{
    EXPECT_EQ(reference(), reference());
}

TEST(GlyphTest, Equality_IsFalseWhenTheMetricsDiffer)
{
    Glyph other = reference();
    other.metrics.advance = 19;

    EXPECT_NE(reference(), other);
}

TEST(GlyphTest, Equality_IsFalseWhenTheCoverageDiffers)
{
    Glyph other = reference();
    other.coverage.samples[0] = 0;

    EXPECT_NE(reference(), other);
}
