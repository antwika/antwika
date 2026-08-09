#include <gtest/gtest.h>

#include <vector>

#include "EntropyIndex.hpp"
#include "antwika/wfc/Domain.hpp"

using antwika::wfc::Domain;
using antwika::wfc::detail::EntropyIndex;

TEST(EntropyIndexTest, PickNext_TakesTheLowestCandidateCount)
{
    std::vector<Domain> wave{Domain(4), Domain(2), Domain(3)};
    EntropyIndex entropyIndex(wave, {});

    ASSERT_TRUE(entropyIndex.pickNext().has_value());
    EXPECT_EQ(*entropyIndex.pickNext(), 1U);
}

TEST(EntropyIndexTest, PickNext_BreaksTiesByLowestIndex)
{
    std::vector<Domain> wave{Domain(3), Domain(3), Domain(3)};
    EntropyIndex entropyIndex(wave, {});

    ASSERT_TRUE(entropyIndex.pickNext().has_value());
    EXPECT_EQ(*entropyIndex.pickNext(), 0U);
}

TEST(EntropyIndexTest, PickNext_ExcludesSingletonAndEmptyCells)
{
    std::vector<Domain> wave{
        Domain::singleton(0, 3), Domain(0), Domain(3)};
    EntropyIndex entropyIndex(wave, {});

    ASSERT_TRUE(entropyIndex.pickNext().has_value());
    EXPECT_EQ(*entropyIndex.pickNext(), 2U);
}

TEST(EntropyIndexTest, PickNext_ReturnsNothingWhenAllDetermined)
{
    std::vector<Domain> wave{
        Domain::singleton(0, 3), Domain::singleton(1, 3)};
    EntropyIndex entropyIndex(wave, {});

    EXPECT_FALSE(entropyIndex.pickNext().has_value());
}

TEST(EntropyIndexTest, Update_StaysConsistentAcrossRewind)
{
    std::vector<Domain> wave{Domain(3), Domain(3)};
    EntropyIndex entropyIndex(wave, {});

    wave[1].remove(0);
    entropyIndex.update(1, wave[1]);
    ASSERT_TRUE(entropyIndex.pickNext().has_value());
    EXPECT_EQ(*entropyIndex.pickNext(), 1U);

    wave[1].add(0);
    entropyIndex.update(1, wave[1]);
    ASSERT_TRUE(entropyIndex.pickNext().has_value());
    EXPECT_EQ(*entropyIndex.pickNext(), 0U);

    wave[0].restrictTo(0);
    entropyIndex.update(0, wave[0]);
    ASSERT_TRUE(entropyIndex.pickNext().has_value());
    EXPECT_EQ(*entropyIndex.pickNext(), 1U);
}
