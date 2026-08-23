#include <gtest/gtest.h>

#include "antwika/font/Coverage.hpp"
#include "antwika/font/FontError.hpp"

using antwika::font::Coverage;
using antwika::font::FontError;

namespace
{
    Coverage getTwoByTwo()
    {
        return Coverage{
            .width = 2, .height = 2, .samples = {1, 2, 3, 4}};
    }
}

TEST(CoverageTest, IsValid_TrueWhenTheSamplesMatchTheSize)
{
    EXPECT_TRUE(getTwoByTwo().isValid());
}

TEST(CoverageTest, IsValid_TrueForAnEmptyMask)
{
    EXPECT_TRUE(Coverage{}.isValid());
}

TEST(CoverageTest, IsValid_FalseWhenSamplesAreMissing)
{
    Coverage maskCoverage = getTwoByTwo();
    maskCoverage.samples.pop_back();

    EXPECT_FALSE(maskCoverage.isValid());
}

TEST(CoverageTest, At_ReadsRowByRowFromTheTopLeft)
{
    const Coverage maskCoverage = getTwoByTwo();

    EXPECT_EQ(maskCoverage.getEntryAt(0, 0), 1);
    EXPECT_EQ(maskCoverage.getEntryAt(1, 0), 2);
    EXPECT_EQ(maskCoverage.getEntryAt(0, 1), 3);
    EXPECT_EQ(maskCoverage.getEntryAt(1, 1), 4);
}

TEST(CoverageTest, At_RefusesAColumnPastTheRight)
{
    EXPECT_THROW((void)getTwoByTwo().getEntryAt(2, 0), FontError);
}

TEST(CoverageTest, At_RefusesARowPastTheBottom)
{
    EXPECT_THROW((void)getTwoByTwo().getEntryAt(0, 2), FontError);
}

TEST(CoverageTest, At_RefusesAnIncompleteMask)
{
    Coverage maskCoverage = getTwoByTwo();
    maskCoverage.samples.pop_back();

    EXPECT_THROW((void)maskCoverage.getEntryAt(0, 0), FontError);
}

TEST(CoverageTest, OperatorEquals_IsTrueForTheSameSizeAndSamples)
{
    EXPECT_EQ(getTwoByTwo(), getTwoByTwo());
}

TEST(CoverageTest, OperatorEquals_IsFalseWhenTheWidthDiffers)
{
    Coverage otherCoverage = getTwoByTwo();
    otherCoverage.width = 4;

    EXPECT_NE(getTwoByTwo(), otherCoverage);
}

TEST(CoverageTest, OperatorEquals_IsFalseWhenTheHeightDiffers)
{
    Coverage otherCoverage = getTwoByTwo();
    otherCoverage.height = 4;

    EXPECT_NE(getTwoByTwo(), otherCoverage);
}

TEST(CoverageTest, OperatorEquals_IsFalseWhenASampleDiffers)
{
    Coverage otherCoverage = getTwoByTwo();
    otherCoverage.samples[3] = 40;

    EXPECT_NE(getTwoByTwo(), otherCoverage);
}
