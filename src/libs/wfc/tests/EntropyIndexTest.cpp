#include <gtest/gtest.h>

#include <vector>

#include "EntropyIndex.hpp"
#include "antwika/wfc/Domain.hpp"

using antwika::wfc::Domain;
using antwika::wfc::detail::EntropyIndex;

TEST(EntropyIndexTest, PickNext_TakesTheLowestCandidateCount)
{
    std::vector<Domain> waveDomains{Domain(4), Domain(2), Domain(3)};
    EntropyIndex entropyIndex(waveDomains, {});

    ASSERT_TRUE(entropyIndex.pickNext().has_value());
    EXPECT_EQ(*entropyIndex.pickNext(), 1U);
}

TEST(EntropyIndexTest, PickNext_BreaksTiesByLowestIndex)
{
    std::vector<Domain> waveDomains{Domain(3), Domain(3), Domain(3)};
    EntropyIndex entropyIndex(waveDomains, {});

    ASSERT_TRUE(entropyIndex.pickNext().has_value());
    EXPECT_EQ(*entropyIndex.pickNext(), 0U);
}

TEST(EntropyIndexTest, PickNext_ExcludesSingletonAndEmptyCells)
{
    std::vector<Domain> waveDomains{
        Domain::createSingleton(0, 3), Domain(0), Domain(3)};
    EntropyIndex entropyIndex(waveDomains, {});

    ASSERT_TRUE(entropyIndex.pickNext().has_value());
    EXPECT_EQ(*entropyIndex.pickNext(), 2U);
}

TEST(EntropyIndexTest, PickNext_ReturnsNothingWhenAllDetermined)
{
    std::vector<Domain> waveDomains{
        Domain::createSingleton(0, 3), Domain::createSingleton(1, 3)};
    EntropyIndex entropyIndex(waveDomains, {});

    EXPECT_FALSE(entropyIndex.pickNext().has_value());
}

TEST(EntropyIndexTest, Update_StaysConsistentAcrossRewind)
{
    std::vector<Domain> waveDomains{Domain(3), Domain(3)};
    EntropyIndex entropyIndex(waveDomains, {});

    waveDomains[1].remove(0);
    entropyIndex.update(1, waveDomains[1]);
    ASSERT_TRUE(entropyIndex.pickNext().has_value());
    EXPECT_EQ(*entropyIndex.pickNext(), 1U);

    waveDomains[1].add(0);
    entropyIndex.update(1, waveDomains[1]);
    ASSERT_TRUE(entropyIndex.pickNext().has_value());
    EXPECT_EQ(*entropyIndex.pickNext(), 0U);

    waveDomains[0].restrictTo(0);
    entropyIndex.update(0, waveDomains[0]);
    ASSERT_TRUE(entropyIndex.pickNext().has_value());
    EXPECT_EQ(*entropyIndex.pickNext(), 1U);
}
