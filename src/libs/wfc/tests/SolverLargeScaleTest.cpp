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
        CompatibilityTable table(4);
        for (std::size_t value = 0; value < 4; ++value)
        {
            table.set(value, value, false);
        }
        return table;
    }
}

TEST(SolverLargeScaleTest, Solve_CompletesThousandsOfCells)
{
    constexpr std::size_t kLength = 4000;
    std::vector<Domain> wave;
    wave.reserve(kLength);
    for (std::size_t i = 0; i < kLength; ++i)
    {
        wave.emplace_back(4);
    }

    std::vector<AdjacencyConstraint> constraints;
    constraints.reserve(kLength - 1);
    for (std::size_t i = 0; i + 1 < kLength; ++i)
    {
        constraints.emplace_back(i, i + 1, makeNeighboursDifferTable());
    }

    const auto constraintRefs = referencesTo(constraints);

    Solver solver(wave, constraintRefs);
    const auto result = solver.solve();

    ASSERT_EQ(result.outcome, SolveOutcome::Solved);
    ASSERT_EQ(result.assignment.size(), kLength);

    ASSERT_EQ(result.assignment[0], 0U);
    ASSERT_EQ(result.assignment[1], 1U);

    const CompatibilityTable table = makeNeighboursDifferTable();
    for (std::size_t i = 0; i + 1 < kLength; ++i)
    {
        EXPECT_TRUE(table.compatible(
            result.assignment[i], result.assignment[i + 1]));
    }
}
