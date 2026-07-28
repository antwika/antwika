#include <antwika/wfc/Solver.hpp>

#include <vector>

#include <gtest/gtest.h>

#include <antwika/wfc/AllDifferentConstraint.hpp>
#include <antwika/wfc/Domain.hpp>
#include <antwika/wfc/SolveResult.hpp>
#include <antwika/wfc/WfcError.hpp>

using antwika::wfc::AllDifferentConstraint;
using antwika::wfc::Domain;
using antwika::wfc::SolveOutcome;
using antwika::wfc::Solver;
using antwika::wfc::WfcError;

TEST(SolverConstructionTest, EmptyWaveSolvesImmediately)
{
    std::vector<Domain> wave;
    Solver solver(wave, {});
    const auto result = solver.solve();

    EXPECT_EQ(result.outcome, SolveOutcome::Solved);
    EXPECT_TRUE(result.assignment.empty());
}

TEST(SolverConstructionTest, MismatchedAlphabetSizesThrows)
{
    std::vector<Domain> wave{Domain(3), Domain(4)};

    EXPECT_THROW(
        { Solver solver(wave, {}); }, WfcError);
}

TEST(SolverConstructionTest, OutOfRangeConstraintCellIndexThrows)
{
    std::vector<Domain> wave{Domain(3), Domain(3)};
    AllDifferentConstraint outOfRange({0, 5});

    EXPECT_THROW(
        { Solver solver(wave, {std::cref(outOfRange)}); }, WfcError);
}
