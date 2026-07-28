#include "EntropyIndex.hpp"

#include <vector>

#include <gtest/gtest.h>

#include "antwika/wfc/Domain.hpp"

using antwika::wfc::Domain;
using antwika::wfc::detail::EntropyIndex;

namespace
{
    // cell0: 3 candidates {0, 1, 2}; cell1: 2 candidates {1, 2}.
    std::vector<Domain> makeWave()
    {
        Domain cell0(3);
        Domain cell1(3);
        cell1.remove(0);
        return {cell0, cell1};
    }
} // namespace

TEST(WeightedEntropyTest, UnweightedFollowsPlainCountMrv)
{
    // Plain MRV: fewer candidates wins, so cell1 (2 candidates) beats
    // cell0 (3 candidates), independent of index order.
    EntropyIndex entropyIndex(makeWave(), {});
    EXPECT_EQ(*entropyIndex.pickNext(), 1U);
}

TEST(WeightedEntropyTest, SkewedWeightsCanFlipTheSelection)
{
    // Value 0 is overwhelmingly likely. cell0's distribution is
    // dominated by it despite having one more raw candidate than
    // cell1, so cell0's weighted entropy ends up lower than cell1's --
    // the opposite of the unweighted MRV order above.
    const std::vector<double> weights{1000.0, 1.0, 1.0};
    EntropyIndex entropyIndex(makeWave(), weights);
    EXPECT_EQ(*entropyIndex.pickNext(), 0U);
}

TEST(WeightedEntropyTest, UniformWeightsReproduceUnweightedOrder)
{
    const std::vector<double> uniform{1.0, 1.0, 1.0};
    EntropyIndex weighted(makeWave(), uniform);
    EntropyIndex unweighted(makeWave(), {});

    EXPECT_EQ(*weighted.pickNext(), *unweighted.pickNext());
}
