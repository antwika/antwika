#include <gtest/gtest.h>

#include <vector>

#include <antwika/wfc/AllDifferentConstraint.hpp>
#include <antwika/wfc/SolveResult.hpp>
#include <antwika/wfc/Solver.hpp>

#include "EntropyIndex.hpp"
#include "antwika/wfc/Domain.hpp"

using antwika::wfc::AllDifferentConstraint;
using antwika::wfc::Domain;
using antwika::wfc::SolveOutcome;
using antwika::wfc::Solver;
using antwika::wfc::detail::EntropyIndex;

TEST(EntropyDeterminismTest, PickNext_OrdersExactlyAcrossEveryCount)
{
    std::vector<Domain> wave;
    for (std::size_t remaining = 8; remaining >= 2; --remaining)
    {
        Domain domain(8);
        for (std::size_t value = remaining; value < 8; ++value)
        {
            domain.remove(value);
        }
        wave.push_back(domain);
    }

    EntropyIndex entropyIndex(wave, {});

    for (std::size_t expected = wave.size(); expected > 0; --expected)
    {
        ASSERT_TRUE(entropyIndex.pickNext().has_value());
        EXPECT_EQ(*entropyIndex.pickNext(), expected - 1);
        entropyIndex.update(
            expected - 1, Domain::singleton(0, 8));
    }

    EXPECT_FALSE(entropyIndex.pickNext().has_value());
}

TEST(EntropyDeterminismTest, PickNext_BreaksAWeightedTieOnIndex)
{
    Domain cell0(4);
    cell0.remove(2);
    cell0.remove(3);
    Domain cell1(4);
    cell1.remove(0);
    cell1.remove(1);

    const std::vector<double> weights{1.0, 1.0, 1.0, 1.0 + 1e-5};
    EntropyIndex entropyIndex({cell0, cell1}, weights);

    ASSERT_TRUE(entropyIndex.pickNext().has_value());
    EXPECT_EQ(*entropyIndex.pickNext(), 0U);
}

namespace
{
    std::vector<Domain> waveTheWeightsReorder()
    {
        Domain cell1(3);
        cell1.remove(2);
        return {Domain(3), cell1};
    }
}

TEST(EntropyDeterminismTest, Solve_RepeatsAWeightedAssignment)
{
    AllDifferentConstraint allDifferent({0, 1});
    const std::vector<double> weights{1.0, 1.0, 1000.0};

    Solver solver(waveTheWeightsReorder(), {std::cref(allDifferent)}, weights);
    const auto first = solver.solve();
    const auto second = solver.solve();

    EXPECT_EQ(first.outcome, SolveOutcome::Solved);
    EXPECT_EQ(first.assignment, (std::vector<std::size_t>{0, 1}));
    EXPECT_EQ(second.outcome, SolveOutcome::Solved);
    EXPECT_EQ(second.assignment, (std::vector<std::size_t>{0, 1}));
}

TEST(EntropyDeterminismTest, Solve_OpensOnTheOtherCellWhenUnweighted)
{
    AllDifferentConstraint allDifferent({0, 1});

    Solver solver(waveTheWeightsReorder(), {std::cref(allDifferent)});
    const auto result = solver.solve();

    EXPECT_EQ(result.outcome, SolveOutcome::Solved);
    EXPECT_EQ(result.assignment, (std::vector<std::size_t>{1, 0}));
}
