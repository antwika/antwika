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

TEST(SolverPropagationTest, Solve_FinishesOnNakedSinglesAlone)
{
    std::vector<Domain> waveDomains{
        Domain::createSingleton(0, 3), Domain::createSingleton(1, 3), Domain(3)};
    AllDifferentConstraint allDifferent({0, 1, 2});

    Solver solver(
        waveDomains,
        {std::cref(allDifferent)},
        {},
        SolverLimits{.maxSteps = 0});
    const auto result = solver.getSolveResult();

    EXPECT_EQ(result.outcome, SolveOutcome::Solved);
    EXPECT_EQ(result.assignment, (std::vector<std::size_t>{0, 1, 2}));
}
