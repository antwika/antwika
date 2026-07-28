#include "EntropyIndex.hpp"

#include <vector>

#include <gtest/gtest.h>

#include "antwika/wfc/Domain.hpp"

using antwika::wfc::Domain;
using antwika::wfc::detail::EntropyIndex;

TEST(EntropyIndexTest, UniformWeightsPicksLowestCandidateCount)
{
    std::vector<Domain> wave{Domain(4), Domain(2), Domain(3)};
    EntropyIndex entropyIndex(wave, {});

    ASSERT_TRUE(entropyIndex.pickNext().has_value());
    EXPECT_EQ(*entropyIndex.pickNext(), 1U);
}

TEST(EntropyIndexTest, TiesBrokenByLowestIndex)
{
    std::vector<Domain> wave{Domain(3), Domain(3), Domain(3)};
    EntropyIndex entropyIndex(wave, {});

    EXPECT_EQ(*entropyIndex.pickNext(), 0U);
}

TEST(EntropyIndexTest, SingletonAndEmptyCellsAreExcluded)
{
    std::vector<Domain> wave{
        Domain::singleton(0, 3), Domain(0), Domain(3)};
    EntropyIndex entropyIndex(wave, {});

    ASSERT_TRUE(entropyIndex.pickNext().has_value());
    EXPECT_EQ(*entropyIndex.pickNext(), 2U);
}

TEST(EntropyIndexTest, ReturnsNulloptWhenNothingUndetermined)
{
    std::vector<Domain> wave{
        Domain::singleton(0, 3), Domain::singleton(1, 3)};
    EntropyIndex entropyIndex(wave, {});

    EXPECT_FALSE(entropyIndex.pickNext().has_value());
}

TEST(EntropyIndexTest, CustomWeightsCanChangeSelectionOrder)
{
    // Cell 0: candidates {0, 1}, weights heavily skewed -> low entropy.
    // Cell 1: candidates {0, 1}, equal weights -> higher entropy.
    // Both have the same candidate count, so uniform MRV would tie.
    // The weights make cell 0 strictly lower entropy.
    std::vector<Domain> wave{Domain(2), Domain(2)};
    std::vector<double> weights{100.0, 1.0};
    EntropyIndex entropyIndex(wave, weights);

    EXPECT_EQ(*entropyIndex.pickNext(), 0U);
}

TEST(EntropyIndexTest, UpdateKeepsIndexConsistentAcrossShrinkAndRewind)
{
    std::vector<Domain> wave{Domain(3), Domain(3)};
    EntropyIndex entropyIndex(wave, {});

    wave[0].remove(0);
    entropyIndex.update(0, wave[0]);
    EXPECT_EQ(*entropyIndex.pickNext(), 0U);

    wave[0].add(0);
    entropyIndex.update(0, wave[0]);
    EXPECT_EQ(*entropyIndex.pickNext(), 0U);

    wave[0].restrictTo(0);
    entropyIndex.update(0, wave[0]);
    EXPECT_EQ(*entropyIndex.pickNext(), 1U);
}
