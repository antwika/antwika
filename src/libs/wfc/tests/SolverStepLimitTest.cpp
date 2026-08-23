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
    std::vector<Domain> waveDomains{Domain(2), Domain(2), Domain(2)};
    AllDifferentConstraint abConstraint({0, 1});
    AllDifferentConstraint bcConstraint({1, 2});
    AllDifferentConstraint acConstraint({0, 2});

    SolverLimits limits{.maxSteps = 1};
    Solver solver(
        waveDomains, {
            std::cref(abConstraint),
            std::cref(bcConstraint),
            std::cref(acConstraint)}, {}, limits);
    const auto result = solver.getSolve();

    EXPECT_EQ(result.outcome, SolveOutcome::LimitExceeded);
    EXPECT_TRUE(result.assignment.empty());
}

TEST(SolverStepLimitTest, Solve_SpendsItsFirstChoiceOnABudgetOfOne)
{
    std::vector<Domain> waveDomains{Domain(2), Domain(2)};
    AllDifferentConstraint allDifferent({0, 1});

    SolverLimits limits{.maxSteps = 1};
    Solver solver(waveDomains, {std::cref(allDifferent)}, {}, limits);
    const auto result = solver.getSolve();

    EXPECT_EQ(result.outcome, SolveOutcome::Solved);
    EXPECT_EQ(result.assignment, (std::vector<std::size_t>{0, 1}));
}

TEST(SolverStepLimitTest, Solve_StillSolvesOnAGenerousBudget)
{
    std::vector<Domain> waveDomains{
        Domain::createSingleton(0, 3), Domain::createSingleton(1, 3), Domain(3)};
    AllDifferentConstraint allDifferent({0, 1, 2});

    SolverLimits limits{.maxSteps = 1000};
    Solver solver(waveDomains, {std::cref(allDifferent)}, {}, limits);
    const auto result = solver.getSolve();

    EXPECT_EQ(result.outcome, SolveOutcome::Solved);
    EXPECT_EQ(result.assignment, (std::vector<std::size_t>{0, 1, 2}));
}
