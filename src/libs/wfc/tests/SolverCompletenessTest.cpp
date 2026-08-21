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
    std::vector<Domain> waveDomains{Domain(2), Domain(2), Domain(2)};
    AllDifferentConstraint abConstraint({0, 1});
    AllDifferentConstraint bcConstraint({1, 2});
    AllDifferentConstraint acConstraint({0, 2});

    Solver solver(
        waveDomains, {
            std::cref(abConstraint),
            std::cref(bcConstraint),
            std::cref(acConstraint)});
    const auto result = solver.solve();

    EXPECT_EQ(result.outcome, SolveOutcome::Unsatisfiable);
    EXPECT_TRUE(result.assignment.empty());
}

TEST(SolverCompletenessTest, Solve_ReportsUnsatisfiableBeforeAnyChoice)
{
    std::vector<Domain> waveDomains{
        Domain::singleton(0, 2), Domain::singleton(0, 2)};
    AllDifferentConstraint allDifferent({0, 1});

    Solver solver(waveDomains, {std::cref(allDifferent)});
    const auto result = solver.solve();

    EXPECT_EQ(result.outcome, SolveOutcome::Unsatisfiable);
    EXPECT_TRUE(result.assignment.empty());
}
