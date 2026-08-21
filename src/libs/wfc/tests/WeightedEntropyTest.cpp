#include <gtest/gtest.h>

#include <vector>

#include "EntropyIndex.hpp"
#include "antwika/wfc/Domain.hpp"

using antwika::wfc::Domain;
using antwika::wfc::detail::EntropyIndex;

namespace
{
    std::vector<Domain> makeWave()
    {
        Domain cell0Domain(3);
        Domain cell1Domain(3);
        cell1Domain.remove(0);
        return {cell0Domain, cell1Domain};
    }
}

TEST(WeightedEntropyTest, PickNext_FollowsPlainCountWhenUnweighted)
{
    EntropyIndex entropyIndex(makeWave(), {});
    ASSERT_TRUE(entropyIndex.pickNext().has_value());
    EXPECT_EQ(*entropyIndex.pickNext(), 1U);
}

TEST(WeightedEntropyTest, PickNext_FlipsTheSelectionOnSkewedWeights)
{
    const std::vector<double> weights{1000.0, 1.0, 1.0};
    EntropyIndex entropyIndex(makeWave(), weights);
    ASSERT_TRUE(entropyIndex.pickNext().has_value());
    EXPECT_EQ(*entropyIndex.pickNext(), 0U);
}

TEST(WeightedEntropyTest, PickNext_MatchesUnweightedOnUniformWeights)
{
    const std::vector<double> uniform{1.0, 1.0, 1.0};
    EntropyIndex weightedIndex(makeWave(), uniform);
    EntropyIndex unweightedIndex(makeWave(), {});

    ASSERT_TRUE(weightedIndex.pickNext().has_value());
    ASSERT_TRUE(unweightedIndex.pickNext().has_value());
    EXPECT_EQ(*weightedIndex.pickNext(), 1U);
    EXPECT_EQ(*unweightedIndex.pickNext(), 1U);
}
