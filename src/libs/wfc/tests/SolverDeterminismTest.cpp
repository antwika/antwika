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

TEST(SolverDeterminismTest, Solve_MatchesForOneSolverRunTwice)
{
    std::vector<Domain> wave{Domain(2), Domain(2)};
    AllDifferentConstraint allDifferent({0, 1});

    Solver solver(wave, {std::cref(allDifferent)});
    const auto first = solver.solve();
    const auto second = solver.solve();

    EXPECT_EQ(first.outcome, SolveOutcome::Solved);
    EXPECT_EQ(first.assignment, (std::vector<std::size_t>{0, 1}));
    EXPECT_EQ(second.outcome, SolveOutcome::Solved);
    EXPECT_EQ(second.assignment, (std::vector<std::size_t>{0, 1}));
}

TEST(SolverDeterminismTest, Solve_MatchesAcrossTwoSolvers)
{
    std::vector<Domain> waveA{Domain(2), Domain(2)};
    std::vector<Domain> waveB{Domain(2), Domain(2)};
    AllDifferentConstraint allDifferentA({0, 1});
    AllDifferentConstraint allDifferentB({0, 1});

    Solver solverA(waveA, {std::cref(allDifferentA)});
    Solver solverB(waveB, {std::cref(allDifferentB)});

    const auto resultA = solverA.solve();
    const auto resultB = solverB.solve();

    EXPECT_EQ(resultA.outcome, SolveOutcome::Solved);
    EXPECT_EQ(resultA.assignment, (std::vector<std::size_t>{0, 1}));
    EXPECT_EQ(resultB.outcome, SolveOutcome::Solved);
    EXPECT_EQ(resultB.assignment, (std::vector<std::size_t>{0, 1}));
}
