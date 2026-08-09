#include <gtest/gtest.h>

#include <vector>

#include <antwika/wfc/Solver.hpp>
#include <antwika/wfc/AllDifferentConstraint.hpp>
#include <antwika/wfc/Domain.hpp>
#include <antwika/wfc/SolveResult.hpp>
#include <antwika/wfc/SolverLimits.hpp>

using antwika::wfc::AllDifferentConstraint;
using antwika::wfc::Domain;
using antwika::wfc::SolveOutcome;
using antwika::wfc::Solver;
using antwika::wfc::SolverLimits;

TEST(SolverStepLimitTest, Solve_ReportsLimitExceededOnATinyBudget)
{
    std::vector<Domain> wave{Domain(2), Domain(2), Domain(2)};
    AllDifferentConstraint ab({0, 1});
    AllDifferentConstraint bc({1, 2});
    AllDifferentConstraint ac({0, 2});

    SolverLimits limits{.maxSteps = 1};
    Solver solver(
        wave, {std::cref(ab), std::cref(bc), std::cref(ac)}, {}, limits);
    const auto result = solver.solve();

    EXPECT_EQ(result.outcome, SolveOutcome::LimitExceeded);
    EXPECT_TRUE(result.assignment.empty());
}

TEST(SolverStepLimitTest, Solve_SpendsItsFirstChoiceOnABudgetOfOne)
{
    std::vector<Domain> wave{Domain(2), Domain(2)};
    AllDifferentConstraint allDifferent({0, 1});

    SolverLimits limits{.maxSteps = 1};
    Solver solver(wave, {std::cref(allDifferent)}, {}, limits);
    const auto result = solver.solve();

    EXPECT_EQ(result.outcome, SolveOutcome::Solved);
    EXPECT_EQ(result.assignment, (std::vector<std::size_t>{0, 1}));
}

TEST(SolverStepLimitTest, Solve_StillSolvesOnAGenerousBudget)
{
    std::vector<Domain> wave{
        Domain::singleton(0, 3), Domain::singleton(1, 3), Domain(3)};
    AllDifferentConstraint allDifferent({0, 1, 2});

    SolverLimits limits{.maxSteps = 1000};
    Solver solver(wave, {std::cref(allDifferent)}, {}, limits);
    const auto result = solver.solve();

    EXPECT_EQ(result.outcome, SolveOutcome::Solved);
    EXPECT_EQ(result.assignment, (std::vector<std::size_t>{0, 1, 2}));
}
