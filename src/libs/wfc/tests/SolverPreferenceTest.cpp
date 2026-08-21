#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <optional>
#include <vector>

#include <antwika/wfc/AdjacencyConstraint.hpp>
#include <antwika/wfc/CompatibilityTable.hpp>
#include <antwika/wfc/Domain.hpp>
#include <antwika/wfc/SolveResult.hpp>
#include <antwika/wfc/Solver.hpp>
#include <antwika/wfc/WfcError.hpp>

using antwika::wfc::AdjacencyConstraint;
using antwika::wfc::CompatibilityTable;
using antwika::wfc::Domain;
using antwika::wfc::SolveOutcome;
using antwika::wfc::Solver;
using antwika::wfc::WfcError;

TEST(SolverPreferenceTest, Solve_TakesThePreferredValueWhereItIsFree)
{
    std::vector<Domain> waveDomains{Domain(3), Domain(3)};

    const auto solution =
        Solver(
            waveDomains,
            {},
            {},
            {},
            {std::optional<std::size_t>{2},
             std::optional<std::size_t>{1}})
            .solve();

    ASSERT_EQ(solution.outcome, SolveOutcome::Solved);
    EXPECT_EQ(solution.assignment[0], 2U);
    EXPECT_EQ(solution.assignment[1], 1U);
}

TEST(SolverPreferenceTest, Solve_PrefersNothingWhereNothingIsAsked)
{
    std::vector<Domain> waveDomains{Domain(3), Domain(3)};

    const auto solution =
        Solver(
            waveDomains,
            {},
            {},
            {},
            {std::optional<std::size_t>{2}, std::nullopt})
            .solve();
    const auto bare = Solver(waveDomains, {}).solve();

    ASSERT_EQ(solution.outcome, SolveOutcome::Solved);
    ASSERT_EQ(bare.outcome, SolveOutcome::Solved);
    EXPECT_EQ(solution.assignment[0], 2U);
    EXPECT_EQ(solution.assignment[1], bare.assignment[1]);
}

TEST(SolverPreferenceTest, Solve_LetsGoOfAPreferenceTheConstraintsRefuse)
{
    CompatibilityTable sameTable(3);

    for (std::size_t left = 0; left < 3; ++left)
    {
        for (std::size_t right = 0; right < 3; ++right)
        {
            sameTable.set(left, right, left == right);
        }
    }

    std::vector<Domain> waveDomains{Domain(3), Domain(3)};

    waveDomains[1].remove(1);
    waveDomains[1].remove(2);

    const AdjacencyConstraint agreeConstraint(0, 1, sameTable);
    const auto solution =
        Solver(
            waveDomains,
            {agreeConstraint},
            {},
            {},
            {std::optional<std::size_t>{2}, std::nullopt})
            .solve();

    ASSERT_EQ(solution.outcome, SolveOutcome::Solved);
    EXPECT_EQ(solution.assignment[0], 0U);
    EXPECT_EQ(solution.assignment[1], 0U);
}

TEST(SolverPreferenceTest, Solver_RefusesPreferencesOfTheWrongCount)
{
    std::vector<Domain> waveDomains{Domain(3), Domain(3)};

    EXPECT_THROW(
        Solver(
            waveDomains,
            {},
            {},
            {},
            {std::optional<std::size_t>{1}}),
        WfcError);
}
