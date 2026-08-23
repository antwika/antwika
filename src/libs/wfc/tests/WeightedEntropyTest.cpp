#include <gtest/gtest.h>

#include <vector>

#include "EntropyIndex.hpp"
#include "antwika/wfc/Domain.hpp"

using antwika::wfc::Domain;
using antwika::wfc::detail::EntropyIndex;

namespace
{
    std::vector<Domain> createWave()
    {
        Domain cell0Domain(3);
        Domain cell1Domain(3);
        cell1Domain.remove(0);
        return {cell0Domain, cell1Domain};
    }
}

TEST(WeightedEntropyTest, PickNext_FollowsPlainCountWhenUnweighted)
{
    EntropyIndex entropyIndex(createWave(), {});
    ASSERT_TRUE(entropyIndex.getPickNext().has_value());
    EXPECT_EQ(*entropyIndex.getPickNext(), 1U);
}

TEST(WeightedEntropyTest, PickNext_FlipsTheSelectionOnSkewedWeights)
{
    const std::vector<double> weights{1000.0, 1.0, 1.0};
    EntropyIndex entropyIndex(createWave(), weights);
    ASSERT_TRUE(entropyIndex.getPickNext().has_value());
    EXPECT_EQ(*entropyIndex.getPickNext(), 0U);
}

TEST(WeightedEntropyTest, PickNext_MatchesUnweightedOnUniformWeights)
{
    const std::vector<double> uniform{1.0, 1.0, 1.0};
    EntropyIndex weightedIndex(createWave(), uniform);
    EntropyIndex unweightedIndex(createWave(), {});

    ASSERT_TRUE(weightedIndex.getPickNext().has_value());
    ASSERT_TRUE(unweightedIndex.getPickNext().has_value());
    EXPECT_EQ(*weightedIndex.getPickNext(), 1U);
    EXPECT_EQ(*unweightedIndex.getPickNext(), 1U);
}
