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

TEST(SolverPropagationTest, NakedSinglesAloneSolveTheWave)
{
    std::vector<Domain> wave{
        Domain::singleton(0, 3), Domain::singleton(1, 3), Domain(3)};
    AllDifferentConstraint allDifferent({0, 1, 2});

    Solver solver(wave, {std::cref(allDifferent)});
    const auto result = solver.solve();

    EXPECT_EQ(result.outcome, SolveOutcome::Solved);
    EXPECT_EQ(result.assignment, (std::vector<std::size_t>{0, 1, 2}));
}
