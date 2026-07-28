#include <antwika/wfc/Solver.hpp>

#include <vector>

#include <gtest/gtest.h>

#include <antwika/wfc/AllDifferentConstraint.hpp>
#include <antwika/wfc/Domain.hpp>
#include <antwika/wfc/SolveResult.hpp>

using antwika::wfc::AllDifferentConstraint;
using antwika::wfc::Domain;
using antwika::wfc::SolveOutcome;
using antwika::wfc::Solver;

TEST(SolverCompletenessTest, UnsatisfiableOnlyAfterFullExhaustion)
{
    // 3 mutually-different cells, only 2 symbols available: pigeonhole
    // makes this unsatisfiable, but only provably so after both
    // candidates at the first choice point (and their consequences)
    // have genuinely been tried.
    std::vector<Domain> wave{Domain(2), Domain(2), Domain(2)};
    AllDifferentConstraint ab({0, 1});
    AllDifferentConstraint bc({1, 2});
    AllDifferentConstraint ac({0, 2});

    Solver solver(
        wave, {std::cref(ab), std::cref(bc), std::cref(ac)});
    const auto result = solver.solve();

    EXPECT_EQ(result.outcome, SolveOutcome::Unsatisfiable);
    EXPECT_TRUE(result.assignment.empty());
}

TEST(SolverCompletenessTest, SolvableWaveReturnsTheUniqueSolution)
{
    std::vector<Domain> wave{
        Domain::singleton(0, 3), Domain::singleton(1, 3), Domain(3)};
    AllDifferentConstraint allDifferent({0, 1, 2});

    Solver solver(wave, {std::cref(allDifferent)});
    const auto result = solver.solve();

    EXPECT_EQ(result.outcome, SolveOutcome::Solved);
    EXPECT_EQ(result.assignment, (std::vector<std::size_t>{0, 1, 2}));
}
