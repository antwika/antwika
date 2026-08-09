#include <gtest/gtest.h>

#include "antwika/font/FontMetrics.hpp"

using antwika::font::FontMetrics;

namespace
{
    constexpr FontMetrics kReference{
        .ascent = 16, .descent = -4, .lineGap = 2, .lineHeight = 22};
}

TEST(FontMetricsTest, Ctor_StartsWithNoHeightAndNoRoomAroundIt)
{
    constexpr FontMetrics fresh;

    EXPECT_EQ(fresh.ascent, 0);
    EXPECT_EQ(fresh.descent, 0);
    EXPECT_EQ(fresh.lineGap, 0);
    EXPECT_EQ(fresh.lineHeight, 0);
}

TEST(FontMetricsTest, OperatorEquals_IsTrueForTheSameFourNumbers)
{
    constexpr FontMetrics same{
        .ascent = 16, .descent = -4, .lineGap = 2, .lineHeight = 22};

    EXPECT_EQ(kReference, same);
}

TEST(FontMetricsTest, OperatorEquals_IsFalseWhenTheAscentDiffers)
{
    constexpr FontMetrics other{
        .ascent = 17, .descent = -4, .lineGap = 2, .lineHeight = 22};

    EXPECT_NE(kReference, other);
}

TEST(FontMetricsTest, OperatorEquals_IsFalseWhenTheDescentDiffers)
{
    constexpr FontMetrics other{
        .ascent = 16, .descent = -5, .lineGap = 2, .lineHeight = 22};

    EXPECT_NE(kReference, other);
}

TEST(FontMetricsTest, OperatorEquals_IsFalseWhenTheLineGapDiffers)
{
    constexpr FontMetrics other{
        .ascent = 16, .descent = -4, .lineGap = 3, .lineHeight = 22};

    EXPECT_NE(kReference, other);
}

TEST(FontMetricsTest, OperatorEquals_IsFalseWhenTheLineHeightDiffers)
{
    constexpr FontMetrics other{
        .ascent = 16, .descent = -4, .lineGap = 2, .lineHeight = 23};

    EXPECT_NE(kReference, other);
}
