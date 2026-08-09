#include <gtest/gtest.h>

#include "antwika/ui/Sizing.hpp"

using antwika::ui::fixedSize;
using antwika::ui::kFit;
using antwika::ui::kGrow;
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
    const Sizing sizing = kFit;

    EXPECT_EQ(SizeMode::Fit, sizing.mode);
    EXPECT_EQ(0U, sizing.pixels);
}

TEST(SizingTest, Grow_AsksToShareWhatIsLeftOver)
{
    const Sizing sizing = kGrow;

    EXPECT_EQ(SizeMode::Grow, sizing.mode);
    EXPECT_EQ(0U, sizing.pixels);
}

TEST(SizingTest, OperatorEquals_IsTrueForTheSameModeAndPixels)
{
    const auto left = fixedSize(40);
    const Sizing right{.mode = SizeMode::Fixed, .pixels = 40};

    EXPECT_EQ(left, right);
}

TEST(SizingTest, OperatorEquals_IsFalseWhenTheModeDiffers)
{
    const Sizing fit = kFit;
    const Sizing grow = kGrow;

    EXPECT_NE(fit, grow);
}

TEST(SizingTest, OperatorEquals_IsFalseWhenThePixelsDiffer)
{
    const auto left = fixedSize(40);
    const auto right = fixedSize(41);

    EXPECT_NE(left, right);
}
