#include <gtest/gtest.h>

#include "antwika/font/Coverage.hpp"
#include "antwika/font/FontError.hpp"

using antwika::font::Coverage;
using antwika::font::FontError;

namespace
{
    Coverage twoByTwo()
    {
        return Coverage{
            .width = 2, .height = 2, .samples = {1, 2, 3, 4}};
    }
}

TEST(CoverageTest, IsValid_TrueWhenTheSamplesMatchTheSize)
{
    EXPECT_TRUE(twoByTwo().isValid());
}

TEST(CoverageTest, IsValid_TrueForAnEmptyMask)
{
    EXPECT_TRUE(Coverage{}.isValid());
}

TEST(CoverageTest, IsValid_FalseWhenSamplesAreMissing)
{
    Coverage maskCoverage = twoByTwo();
    maskCoverage.samples.pop_back();

    EXPECT_FALSE(maskCoverage.isValid());
}

TEST(CoverageTest, At_ReadsRowByRowFromTheTopLeft)
{
    const Coverage maskCoverage = twoByTwo();

    EXPECT_EQ(maskCoverage.at(0, 0), 1);
    EXPECT_EQ(maskCoverage.at(1, 0), 2);
    EXPECT_EQ(maskCoverage.at(0, 1), 3);
    EXPECT_EQ(maskCoverage.at(1, 1), 4);
}

TEST(CoverageTest, At_RefusesAColumnPastTheRight)
{
    EXPECT_THROW((void)twoByTwo().at(2, 0), FontError);
}

TEST(CoverageTest, At_RefusesARowPastTheBottom)
{
    EXPECT_THROW((void)twoByTwo().at(0, 2), FontError);
}

TEST(CoverageTest, At_RefusesAnIncompleteMask)
{
    Coverage maskCoverage = twoByTwo();
    maskCoverage.samples.pop_back();

    EXPECT_THROW((void)maskCoverage.at(0, 0), FontError);
}

TEST(CoverageTest, OperatorEquals_IsTrueForTheSameSizeAndSamples)
{
    EXPECT_EQ(twoByTwo(), twoByTwo());
}

TEST(CoverageTest, OperatorEquals_IsFalseWhenTheWidthDiffers)
{
    Coverage otherCoverage = twoByTwo();
    otherCoverage.width = 4;

    EXPECT_NE(twoByTwo(), otherCoverage);
}

TEST(CoverageTest, OperatorEquals_IsFalseWhenTheHeightDiffers)
{
    Coverage otherCoverage = twoByTwo();
    otherCoverage.height = 4;

    EXPECT_NE(twoByTwo(), otherCoverage);
}

TEST(CoverageTest, OperatorEquals_IsFalseWhenASampleDiffers)
{
    Coverage otherCoverage = twoByTwo();
    otherCoverage.samples[3] = 40;

    EXPECT_NE(twoByTwo(), otherCoverage);
}
