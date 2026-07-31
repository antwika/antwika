#include "antwika/ttf/Coverage.hpp"

#include <gtest/gtest.h>

#include "antwika/ttf/TtfError.hpp"

using antwika::ttf::Coverage;
using antwika::ttf::TtfError;

namespace
{
    Coverage twoByTwo()
    {
        return Coverage{
            .width = 2, .height = 2, .samples = {1, 2, 3, 4}};
    }
} // namespace

TEST(CoverageTest, IsComplete_TrueWhenTheSamplesMatchTheSize)
{
    EXPECT_TRUE(twoByTwo().isComplete());
}

// An empty mask is an ordinary answer rather than a broken one.
// A space covers nothing and still has an advance.
TEST(CoverageTest, IsComplete_TrueForAnEmptyMask)
{
    EXPECT_TRUE(Coverage{}.isComplete());
}

TEST(CoverageTest, IsComplete_FalseWhenSamplesAreMissing)
{
    Coverage mask = twoByTwo();
    mask.samples.pop_back();

    EXPECT_FALSE(mask.isComplete());
}

TEST(CoverageTest, At_ReadsRowByRowFromTheTopLeft)
{
    const Coverage mask = twoByTwo();

    EXPECT_EQ(mask.at(0, 0), 1);
    EXPECT_EQ(mask.at(1, 0), 2);
    EXPECT_EQ(mask.at(0, 1), 3);
    EXPECT_EQ(mask.at(1, 1), 4);
}

TEST(CoverageTest, At_RefusesAColumnPastTheRight)
{
    EXPECT_THROW((void)twoByTwo().at(2, 0), TtfError);
}

TEST(CoverageTest, At_RefusesARowPastTheBottom)
{
    EXPECT_THROW((void)twoByTwo().at(0, 2), TtfError);
}

// A mask can declare more samples than it holds.
// Reading one of those is the read that runs off the vector.
TEST(CoverageTest, At_RefusesAnIncompleteMask)
{
    Coverage mask = twoByTwo();
    mask.samples.pop_back();

    EXPECT_THROW((void)mask.at(0, 0), TtfError);
}

TEST(CoverageTest, Equality_IsTrueForTheSameSizeAndSamples)
{
    EXPECT_EQ(twoByTwo(), twoByTwo());
}

TEST(CoverageTest, Equality_IsFalseWhenTheWidthDiffers)
{
    Coverage other = twoByTwo();
    other.width = 4;

    EXPECT_NE(twoByTwo(), other);
}

TEST(CoverageTest, Equality_IsFalseWhenTheHeightDiffers)
{
    Coverage other = twoByTwo();
    other.height = 4;

    EXPECT_NE(twoByTwo(), other);
}

TEST(CoverageTest, Equality_IsFalseWhenASampleDiffers)
{
    Coverage other = twoByTwo();
    other.samples[3] = 40;

    EXPECT_NE(twoByTwo(), other);
}
