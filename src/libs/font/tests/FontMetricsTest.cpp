#include <gtest/gtest.h>

#include "antwika/font/FontMetrics.hpp"

using antwika::font::FontMetrics;

namespace
{
    constexpr FontMetrics kReference{
        .ascent = 16, .descent = -4, .lineGap = 2, .lineHeight = 22};
} // namespace

TEST(FontMetricsTest, Equality_IsTrueForTheSameFourNumbers)
{
    constexpr FontMetrics same{
        .ascent = 16, .descent = -4, .lineGap = 2, .lineHeight = 22};

    EXPECT_EQ(kReference, same);
}

TEST(FontMetricsTest, Equality_IsFalseWhenTheAscentDiffers)
{
    constexpr FontMetrics other{
        .ascent = 17, .descent = -4, .lineGap = 2, .lineHeight = 22};

    EXPECT_NE(kReference, other);
}

TEST(FontMetricsTest, Equality_IsFalseWhenTheDescentDiffers)
{
    constexpr FontMetrics other{
        .ascent = 16, .descent = -5, .lineGap = 2, .lineHeight = 22};

    EXPECT_NE(kReference, other);
}

TEST(FontMetricsTest, Equality_IsFalseWhenTheLineGapDiffers)
{
    constexpr FontMetrics other{
        .ascent = 16, .descent = -4, .lineGap = 3, .lineHeight = 22};

    EXPECT_NE(kReference, other);
}

TEST(FontMetricsTest, Equality_IsFalseWhenTheLineHeightDiffers)
{
    constexpr FontMetrics other{
        .ascent = 16, .descent = -4, .lineGap = 2, .lineHeight = 23};

    EXPECT_NE(kReference, other);
}
