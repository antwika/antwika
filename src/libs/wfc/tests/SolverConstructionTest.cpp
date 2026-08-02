#include <antwika/wfc/Solver.hpp>

#include <string>
#include <utility>
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

// Domain is public and mutable, so a caller really can build this.
// It used to reach the end of a solve and throw about a singleton.
// Which read as an internal bug rather than as the input it is.
TEST(SolverConstructionTest, AnEmptyInitialDomainThrows)
{
    Domain empty(3);
    empty.remove(0);
    empty.remove(1);
    empty.remove(2);
    ASSERT_TRUE(empty.isEmpty());

    std::vector<Domain> wave{Domain(3), std::move(empty)};

    try
    {
        Solver solver(wave, {});
        FAIL() << "expected a WfcError";
    }
    catch (const WfcError &error)
    {
        const std::string message = error.what();
        EXPECT_NE(message.find("empty"), std::string::npos);
        EXPECT_EQ(message.find("singleton"), std::string::npos);
    }
}

TEST(SolverConstructionTest, MismatchedValueWeightsSizeThrows)
{
    std::vector<Domain> wave{Domain(3), Domain(3)};
    const std::vector<double> tooFewWeights{1.0, 1.0};

    EXPECT_THROW(
        { Solver solver(wave, {}, tooFewWeights); }, WfcError);
}

TEST(SolverConstructionTest, ZeroValueWeightThrows)
{
    std::vector<Domain> wave{Domain(3), Domain(3)};
    const std::vector<double> zeroWeight{1.0, 0.0, 1.0};

    EXPECT_THROW(
        { Solver solver(wave, {}, zeroWeight); }, WfcError);
}

TEST(SolverConstructionTest, NegativeValueWeightThrows)
{
    std::vector<Domain> wave{Domain(3), Domain(3)};
    const std::vector<double> negativeWeight{1.0, -1.0, 1.0};

    EXPECT_THROW(
        { Solver solver(wave, {}, negativeWeight); }, WfcError);
}
