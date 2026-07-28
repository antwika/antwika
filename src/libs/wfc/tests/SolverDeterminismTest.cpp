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

TEST(SolverDeterminismTest, SameWaveSolvedTwiceViaOneSolverMatches)
{
    // Two solutions exist: (0,1) and (1,0).
    // This proves the same solution is always picked, not just any.
    std::vector<Domain> wave{Domain(2), Domain(2)};
    AllDifferentConstraint allDifferent({0, 1});

    Solver solver(wave, {std::cref(allDifferent)});
    const auto first = solver.solve();
    const auto second = solver.solve();

    EXPECT_EQ(first.outcome, second.outcome);
    EXPECT_EQ(first.assignment, second.assignment);
    EXPECT_EQ(first.outcome, SolveOutcome::Solved);
    EXPECT_EQ(first.assignment, (std::vector<std::size_t>{0, 1}));
}

TEST(SolverDeterminismTest, TwoIndependentlyConstructedSolversMatch)
{
    std::vector<Domain> waveA{Domain(2), Domain(2)};
    std::vector<Domain> waveB{Domain(2), Domain(2)};
    AllDifferentConstraint allDifferentA({0, 1});
    AllDifferentConstraint allDifferentB({0, 1});

    Solver solverA(waveA, {std::cref(allDifferentA)});
    Solver solverB(waveB, {std::cref(allDifferentB)});

    const auto resultA = solverA.solve();
    const auto resultB = solverB.solve();

    EXPECT_EQ(resultA.outcome, resultB.outcome);
    EXPECT_EQ(resultA.assignment, resultB.assignment);
}
