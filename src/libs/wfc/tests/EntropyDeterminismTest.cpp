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
    std::vector<Domain> waveDomains;
    for (std::size_t remaining = 8; remaining >= 2; --remaining)
    {
        Domain domain(8);
        for (std::size_t value = remaining; value < 8; ++value)
        {
            domain.remove(value);
        }
        waveDomains.push_back(domain);
    }

    EntropyIndex entropyIndex(waveDomains, {});

    for (std::size_t remaining = waveDomains.size(); remaining > 0; --remaining)
    {
        ASSERT_TRUE(entropyIndex.getPickNext().has_value());
        EXPECT_EQ(*entropyIndex.getPickNext(), remaining - 1);
        entropyIndex.update(
            remaining - 1, Domain::createSingleton(0, 8));
    }

    EXPECT_FALSE(entropyIndex.getPickNext().has_value());
}

TEST(EntropyDeterminismTest, PickNext_BreaksAWeightedTieOnIndex)
{
    Domain cell0Domain(4);
    cell0Domain.remove(2);
    cell0Domain.remove(3);
    Domain cell1Domain(4);
    cell1Domain.remove(0);
    cell1Domain.remove(1);

    const std::vector<double> weights{1.0, 1.0, 1.0, 1.0 + 1e-5};
    EntropyIndex entropyIndex({cell0Domain, cell1Domain}, weights);

    ASSERT_TRUE(entropyIndex.getPickNext().has_value());
    EXPECT_EQ(*entropyIndex.getPickNext(), 0U);
}

namespace
{
    std::vector<Domain> getWaveTheWeightsReorder()
    {
        Domain cell1Domain(3);
        cell1Domain.remove(2);
        return {Domain(3), cell1Domain};
    }
}

TEST(EntropyDeterminismTest, Solve_RepeatsAWeightedAssignment)
{
    AllDifferentConstraint allDifferent({0, 1});
    const std::vector<double> weights{1.0, 1.0, 1000.0};

    Solver solver(getWaveTheWeightsReorder(), {std::cref(allDifferent)}, weights);
    const auto first = solver.getSolve();
    const auto second = solver.getSolve();

    EXPECT_EQ(first.outcome, SolveOutcome::Solved);
    EXPECT_EQ(first.assignment, (std::vector<std::size_t>{0, 1}));
    EXPECT_EQ(second.outcome, SolveOutcome::Solved);
    EXPECT_EQ(second.assignment, (std::vector<std::size_t>{0, 1}));
}

TEST(EntropyDeterminismTest, Solve_OpensOnTheOtherCellWhenUnweighted)
{
    AllDifferentConstraint allDifferent({0, 1});

    Solver solver(getWaveTheWeightsReorder(), {std::cref(allDifferent)});
    const auto result = solver.getSolve();

    EXPECT_EQ(result.outcome, SolveOutcome::Solved);
    EXPECT_EQ(result.assignment, (std::vector<std::size_t>{1, 0}));
}
