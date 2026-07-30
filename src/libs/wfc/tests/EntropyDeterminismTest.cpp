#include "EntropyIndex.hpp"

#include <vector>

#include <gtest/gtest.h>

#include <antwika/wfc/AllDifferentConstraint.hpp>
#include <antwika/wfc/SolveResult.hpp>
#include <antwika/wfc/Solver.hpp>

#include "antwika/wfc/Domain.hpp"

using antwika::wfc::AllDifferentConstraint;
using antwika::wfc::Domain;
using antwika::wfc::SolveOutcome;
using antwika::wfc::Solver;
using antwika::wfc::detail::EntropyIndex;

// Which cell is collapsed next decides which solution a wave produces.
// So the ordering these tests pin down is the solver's reproducibility.
// The risk they guard is a key that std::log's last ULP can move.
// CI builds against glibc, LLVM's libm and mingw's CRT.
// None of the three is required to round log the same way.

TEST(EntropyDeterminismTest, UniformOrderingIsExactAcrossEveryCount)
{
    // Under uniform weights the key is the candidate count itself.
    // Every count from 2 up therefore orders exactly.
    // Nothing here depends on how this libm rounded a logarithm.
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

    // The wave was built descending, so the last cell is the smallest.
    for (std::size_t expected = wave.size(); expected > 0; --expected)
    {
        ASSERT_TRUE(entropyIndex.pickNext().has_value());
        EXPECT_EQ(*entropyIndex.pickNext(), expected - 1);
        entropyIndex.update(
            expected - 1, Domain::singleton(0, 8));
    }

    EXPECT_FALSE(entropyIndex.pickNext().has_value());
}

TEST(EntropyDeterminismTest, WeightedKeysWithinOneStepTieOnIndex)
{
    // Cell 0 holds {0, 1} and cell 1 holds {2, 3}.
    // The two weight pairs differ by 1e-5.
    // That puts the entropies about 1.2e-11 apart.
    // Both the 1e-9 step and an ULP of libm disagreement dwarf it.
    // Ordering on the raw double would hand this to cell 1.
    // It would do so on a difference no two toolchains agree on.
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

TEST(EntropyDeterminismTest, WeightedSolveRepeatsItsAssignment)
{
    // Two solutions exist here.
    // So a shifted search order shows up as a different solution.
    // It would not merely be a different route to the same one.
    std::vector<Domain> wave{Domain(2), Domain(2)};
    AllDifferentConstraint allDifferent({0, 1});
    const std::vector<double> weights{3.0, 1.0};

    Solver solver(wave, {std::cref(allDifferent)}, weights);
    const auto first = solver.solve();
    const auto second = solver.solve();

    EXPECT_EQ(first.outcome, SolveOutcome::Solved);
    EXPECT_EQ(first.outcome, second.outcome);
    EXPECT_EQ(first.assignment, second.assignment);
}
