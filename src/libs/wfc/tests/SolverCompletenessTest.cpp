#include <gtest/gtest.h>

#include <vector>

#include <antwika/wfc/Solver.hpp>
#include <antwika/wfc/AllDifferentConstraint.hpp>
#include <antwika/wfc/Domain.hpp>
#include <antwika/wfc/SolveResult.hpp>

using antwika::wfc::AllDifferentConstraint;
using antwika::wfc::Domain;
using antwika::wfc::SolveOutcome;
using antwika::wfc::Solver;

TEST(SolverCompletenessTest, Solve_ReportsUnsatisfiableAfterExhaustion)
{
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

TEST(SolverCompletenessTest, Solve_ReportsUnsatisfiableBeforeAnyChoice)
{
    std::vector<Domain> wave{
        Domain::singleton(0, 2), Domain::singleton(0, 2)};
    AllDifferentConstraint allDifferent({0, 1});

    Solver solver(wave, {std::cref(allDifferent)});
    const auto result = solver.solve();

    EXPECT_EQ(result.outcome, SolveOutcome::Unsatisfiable);
    EXPECT_TRUE(result.assignment.empty());
}
