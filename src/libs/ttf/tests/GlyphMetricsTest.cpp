#include <gtest/gtest.h>

#include "antwika/ttf/GlyphMetrics.hpp"

using antwika::ttf::GlyphMetrics;

namespace
{
    constexpr GlyphMetrics kReference{
        .advance = 18, .bearingX = 2, .bearingY = -15};
} // namespace

TEST(GlyphMetricsTest, Equality_IsTrueForTheSameThreeNumbers)
{
    constexpr GlyphMetrics same{
        .advance = 18, .bearingX = 2, .bearingY = -15};

    EXPECT_EQ(kReference, same);
}

TEST(GlyphMetricsTest, Equality_IsFalseWhenTheAdvanceDiffers)
{
    constexpr GlyphMetrics other{
        .advance = 19, .bearingX = 2, .bearingY = -15};

    EXPECT_NE(kReference, other);
}

TEST(GlyphMetricsTest, Equality_IsFalseWhenTheSideBearingDiffers)
{
    constexpr GlyphMetrics other{
        .advance = 18, .bearingX = 3, .bearingY = -15};

    EXPECT_NE(kReference, other);
}

TEST(GlyphMetricsTest, Equality_IsFalseWhenTheHeightAboveTheBaselineDiffers)
{
    constexpr GlyphMetrics other{
        .advance = 18, .bearingX = 2, .bearingY = -14};

    EXPECT_NE(kReference, other);
}
