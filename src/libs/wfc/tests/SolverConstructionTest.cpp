#include <gtest/gtest.h>

#include <string>
#include <utility>
#include <vector>

#include <antwika/wfc/Solver.hpp>
#include <antwika/wfc/AllDifferentConstraint.hpp>
#include <antwika/wfc/Domain.hpp>
#include <antwika/wfc/SolveResult.hpp>
#include <antwika/wfc/WfcError.hpp>

using antwika::wfc::AllDifferentConstraint;
using antwika::wfc::Domain;
using antwika::wfc::SolveOutcome;
using antwika::wfc::Solver;
using antwika::wfc::WfcError;

TEST(SolverConstructionTest, Solve_SolvesAnEmptyWaveImmediately)
{
    std::vector<Domain> waveDomains;
    Solver solver(waveDomains, {});
    const auto result = solver.getSolve();

    EXPECT_EQ(result.outcome, SolveOutcome::Solved);
    EXPECT_TRUE(result.assignment.empty());
}

TEST(SolverConstructionTest, Ctor_ThrowsOnMismatchedAlphabets)
{
    std::vector<Domain> waveDomains{Domain(3), Domain(4)};

    EXPECT_THROW(
        { Solver solver(waveDomains, {}); }, WfcError);
}

TEST(SolverConstructionTest, Ctor_ThrowsOnAnOutOfRangeCell)
{
    std::vector<Domain> waveDomains{Domain(3), Domain(3)};
    AllDifferentConstraint constraintOutOfRange({0, 5});

    EXPECT_THROW(
        {
            Solver solver(waveDomains, {std::cref(constraintOutOfRange)});
        },
        WfcError);
}

TEST(SolverConstructionTest, Ctor_ThrowsOnAnEmptyInitialDomain)
{
    Domain emptyDomain(3);
    emptyDomain.remove(0);
    emptyDomain.remove(1);
    emptyDomain.remove(2);
    ASSERT_TRUE(emptyDomain.isEmpty());

    std::vector<Domain> waveDomains{Domain(3), std::move(emptyDomain)};

    try
    {
        Solver solver(waveDomains, {});
        FAIL() << "expected a WfcError";
    }
    catch (const WfcError &error)
    {
        const std::string message = error.what();
        EXPECT_NE(message.find("empty"), std::string::npos);
        EXPECT_EQ(message.find("singleton"), std::string::npos);
    }
}

TEST(SolverConstructionTest, Ctor_ThrowsOnMismatchedWeights)
{
    std::vector<Domain> waveDomains{Domain(3), Domain(3)};
    const std::vector<double> tooFewWeights{1.0, 1.0};

    EXPECT_THROW(
        { Solver solver(waveDomains, {}, tooFewWeights); }, WfcError);
}

TEST(SolverConstructionTest, Ctor_ThrowsOnAZeroValueWeight)
{
    std::vector<Domain> waveDomains{Domain(3), Domain(3)};
    const std::vector<double> zeroWeight{1.0, 0.0, 1.0};

    EXPECT_THROW(
        { Solver solver(waveDomains, {}, zeroWeight); }, WfcError);
}

TEST(SolverConstructionTest, Ctor_ThrowsOnANegativeValueWeight)
{
    std::vector<Domain> waveDomains{Domain(3), Domain(3)};
    const std::vector<double> negativeWeight{1.0, -1.0, 1.0};

    EXPECT_THROW(
        { Solver solver(waveDomains, {}, negativeWeight); }, WfcError);
}
