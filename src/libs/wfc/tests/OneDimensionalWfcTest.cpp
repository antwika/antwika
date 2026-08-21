#include <gtest/gtest.h>

#include <vector>

#include <antwika/wfc/Solver.hpp>
#include <antwika/wfc/AdjacencyConstraint.hpp>
#include <antwika/wfc/CompatibilityTable.hpp>
#include <antwika/wfc/ConstraintRefs.hpp>
#include <antwika/wfc/Domain.hpp>
#include <antwika/wfc/SolveResult.hpp>

using antwika::wfc::AdjacencyConstraint;
using antwika::wfc::CompatibilityTable;
using antwika::wfc::Domain;
using antwika::wfc::referencesTo;
using antwika::wfc::SolveOutcome;
using antwika::wfc::Solver;

namespace
{
    CompatibilityTable makeNeighboursDifferTable()
    {
        CompatibilityTable table(3);
        for (std::size_t value = 0; value < 3; ++value)
        {
            table.set(value, value, false);
        }
        return table;
    }

    [[nodiscard]] std::vector<std::size_t> alternating(std::size_t length)
    {
        std::vector<std::size_t> values;
        values.reserve(length);
        for (std::size_t i = 0; i < length; ++i)
        {
            values.push_back(i % 2);
        }
        return values;
    }
}

TEST(OneDimensionalWfcTest, Solve_LeavesOnlyCompatibleNeighbours)
{
    constexpr std::size_t kLength = 10;
    std::vector<Domain> waveDomains;
    for (std::size_t i = 0; i < kLength; ++i)
    {
        waveDomains.emplace_back(3);
    }

    std::vector<AdjacencyConstraint> constraints;
    constraints.reserve(kLength - 1);
    for (std::size_t i = 0; i + 1 < kLength; ++i)
    {
        constraints.emplace_back(i, i + 1, makeNeighboursDifferTable());
    }

    const auto constraintRefs = referencesTo(constraints);

    Solver solver(waveDomains, constraintRefs);
    const auto result = solver.solve();

    ASSERT_EQ(result.outcome, SolveOutcome::Solved);
    ASSERT_EQ(result.assignment.size(), kLength);
    for (std::size_t i = 0; i + 1 < kLength; ++i)
    {
        EXPECT_TRUE(makeNeighboursDifferTable().compatible(
            result.assignment[i], result.assignment[i + 1]));
    }

    EXPECT_EQ(result.assignment, alternating(kLength));
}

TEST(OneDimensionalWfcTest, Solve_RepeatsTheRecordedAssignment)
{
    constexpr std::size_t kLength = 8;
    std::vector<Domain> waveDomains;
    for (std::size_t i = 0; i < kLength; ++i)
    {
        waveDomains.emplace_back(3);
    }

    std::vector<AdjacencyConstraint> constraints;
    constraints.reserve(kLength - 1);
    for (std::size_t i = 0; i + 1 < kLength; ++i)
    {
        constraints.emplace_back(i, i + 1, makeNeighboursDifferTable());
    }

    const auto constraintRefs = referencesTo(constraints);

    Solver solverA(waveDomains, constraintRefs);
    Solver solverB(waveDomains, constraintRefs);

    EXPECT_EQ(solverA.solve().assignment, alternating(kLength));
    EXPECT_EQ(solverB.solve().assignment, alternating(kLength));
}
