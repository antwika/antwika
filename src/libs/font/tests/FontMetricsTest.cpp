#include <gtest/gtest.h>

#include "antwika/font/FontMetrics.hpp"

using antwika::font::FontMetrics;

namespace
{
    constexpr FontMetrics kReferenceMetrics{
        .ascent = 16, .descent = -4, .lineGap = 2, .lineHeight = 22};
}

TEST(FontMetricsTest, Ctor_StartsWithNoHeightAndNoRoomAroundIt)
{
    constexpr FontMetrics freshMetrics;

    EXPECT_EQ(freshMetrics.ascent, 0);
    EXPECT_EQ(freshMetrics.descent, 0);
    EXPECT_EQ(freshMetrics.lineGap, 0);
    EXPECT_EQ(freshMetrics.lineHeight, 0);
}

TEST(FontMetricsTest, OperatorEquals_IsTrueForTheSameFourNumbers)
{
    constexpr FontMetrics sameMetrics{
        .ascent = 16, .descent = -4, .lineGap = 2, .lineHeight = 22};

    EXPECT_EQ(kReferenceMetrics, sameMetrics);
}

TEST(FontMetricsTest, OperatorEquals_IsFalseWhenTheAscentDiffers)
{
    constexpr FontMetrics otherMetrics{
        .ascent = 17, .descent = -4, .lineGap = 2, .lineHeight = 22};

    EXPECT_NE(kReferenceMetrics, otherMetrics);
}

TEST(FontMetricsTest, OperatorEquals_IsFalseWhenTheDescentDiffers)
{
    constexpr FontMetrics otherMetrics{
        .ascent = 16, .descent = -5, .lineGap = 2, .lineHeight = 22};

    EXPECT_NE(kReferenceMetrics, otherMetrics);
}

TEST(FontMetricsTest, OperatorEquals_IsFalseWhenTheLineGapDiffers)
{
    constexpr FontMetrics otherMetrics{
        .ascent = 16, .descent = -4, .lineGap = 3, .lineHeight = 22};

    EXPECT_NE(kReferenceMetrics, otherMetrics);
}

TEST(FontMetricsTest, OperatorEquals_IsFalseWhenTheLineHeightDiffers)
{
    constexpr FontMetrics otherMetrics{
        .ascent = 16, .descent = -4, .lineGap = 2, .lineHeight = 23};

    EXPECT_NE(kReferenceMetrics, otherMetrics);
}
