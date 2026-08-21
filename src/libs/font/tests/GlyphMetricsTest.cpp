#include <gtest/gtest.h>

#include "antwika/font/GlyphMetrics.hpp"

using antwika::font::GlyphMetrics;

namespace
{
    constexpr GlyphMetrics kReferenceMetrics{
        .advance = 18, .bearingX = 2, .bearingY = -15};
}

TEST(GlyphMetricsTest, Ctor_StartsOnThePenWithNoAdvance)
{
    constexpr GlyphMetrics freshMetrics;

    EXPECT_EQ(freshMetrics.advance, 0);
    EXPECT_EQ(freshMetrics.bearingX, 0);
    EXPECT_EQ(freshMetrics.bearingY, 0);
}

TEST(GlyphMetricsTest, OperatorEquals_IsTrueForTheSameThreeNumbers)
{
    constexpr GlyphMetrics sameMetrics{
        .advance = 18, .bearingX = 2, .bearingY = -15};

    EXPECT_EQ(kReferenceMetrics, sameMetrics);
}

TEST(GlyphMetricsTest, OperatorEquals_IsFalseWhenTheAdvanceDiffers)
{
    constexpr GlyphMetrics otherMetrics{
        .advance = 19, .bearingX = 2, .bearingY = -15};

    EXPECT_NE(kReferenceMetrics, otherMetrics);
}

TEST(GlyphMetricsTest, OperatorEquals_IsFalseWhenTheSideBearingDiffers)
{
    constexpr GlyphMetrics otherMetrics{
        .advance = 18, .bearingX = 3, .bearingY = -15};

    EXPECT_NE(kReferenceMetrics, otherMetrics);
}

TEST(GlyphMetricsTest,
     OperatorEquals_IsFalseWhenTheHeightAboveTheBaselineDiffers)
{
    constexpr GlyphMetrics otherMetrics{
        .advance = 18, .bearingX = 2, .bearingY = -14};

    EXPECT_NE(kReferenceMetrics, otherMetrics);
}
