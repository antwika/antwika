#include <gtest/gtest.h>

#include "antwika/font/GlyphMetrics.hpp"

using antwika::font::GlyphMetrics;

namespace
{
    constexpr GlyphMetrics kReference{
        .advance = 18, .bearingX = 2, .bearingY = -15};
}

TEST(GlyphMetricsTest, Ctor_StartsOnThePenWithNoAdvance)
{
    constexpr GlyphMetrics fresh;

    EXPECT_EQ(fresh.advance, 0);
    EXPECT_EQ(fresh.bearingX, 0);
    EXPECT_EQ(fresh.bearingY, 0);
}

TEST(GlyphMetricsTest, OperatorEquals_IsTrueForTheSameThreeNumbers)
{
    constexpr GlyphMetrics same{
        .advance = 18, .bearingX = 2, .bearingY = -15};

    EXPECT_EQ(kReference, same);
}

TEST(GlyphMetricsTest, OperatorEquals_IsFalseWhenTheAdvanceDiffers)
{
    constexpr GlyphMetrics other{
        .advance = 19, .bearingX = 2, .bearingY = -15};

    EXPECT_NE(kReference, other);
}

TEST(GlyphMetricsTest, OperatorEquals_IsFalseWhenTheSideBearingDiffers)
{
    constexpr GlyphMetrics other{
        .advance = 18, .bearingX = 3, .bearingY = -15};

    EXPECT_NE(kReference, other);
}

TEST(GlyphMetricsTest,
     OperatorEquals_IsFalseWhenTheHeightAboveTheBaselineDiffers)
{
    constexpr GlyphMetrics other{
        .advance = 18, .bearingX = 2, .bearingY = -14};

    EXPECT_NE(kReference, other);
}
