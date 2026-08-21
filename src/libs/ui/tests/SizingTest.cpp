#include <gtest/gtest.h>

#include "antwika/ui/Sizing.hpp"

using antwika::ui::fixedSize;
using antwika::ui::kFitSizing;
using antwika::ui::kGrowSizing;
using antwika::ui::SizeMode;
using antwika::ui::Sizing;

TEST(SizingTest, FixedSize_AsksForExactlyThePixelsGiven)
{
    const auto sizing = fixedSize(120);

    EXPECT_EQ(SizeMode::Fixed, sizing.mode);
    EXPECT_EQ(120U, sizing.pixels);
}

TEST(SizingTest, Fit_AsksForTheContentAndNoPixelCount)
{
    const Sizing sizing = kFitSizing;

    EXPECT_EQ(SizeMode::Fit, sizing.mode);
    EXPECT_EQ(0U, sizing.pixels);
}

TEST(SizingTest, Grow_AsksToShareWhatIsLeftOver)
{
    const Sizing sizing = kGrowSizing;

    EXPECT_EQ(SizeMode::Grow, sizing.mode);
    EXPECT_EQ(0U, sizing.pixels);
}

TEST(SizingTest, OperatorEquals_IsTrueForTheSameModeAndPixels)
{
    const auto left = fixedSize(40);
    const Sizing rightSizing{.mode = SizeMode::Fixed, .pixels = 40};

    EXPECT_EQ(left, rightSizing);
}

TEST(SizingTest, OperatorEquals_IsFalseWhenTheModeDiffers)
{
    const Sizing fitSizing = kFitSizing;
    const Sizing growSizing = kGrowSizing;

    EXPECT_NE(fitSizing, growSizing);
}

TEST(SizingTest, OperatorEquals_IsFalseWhenThePixelsDiffer)
{
    const auto left = fixedSize(40);
    const auto right = fixedSize(41);

    EXPECT_NE(left, right);
}
