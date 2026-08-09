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
        Domain cell0(3);
        Domain cell1(3);
        cell1.remove(0);
        return {cell0, cell1};
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
    EntropyIndex weighted(makeWave(), uniform);
    EntropyIndex unweighted(makeWave(), {});

    ASSERT_TRUE(weighted.pickNext().has_value());
    ASSERT_TRUE(unweighted.pickNext().has_value());
    EXPECT_EQ(*weighted.pickNext(), 1U);
    EXPECT_EQ(*unweighted.pickNext(), 1U);
}
