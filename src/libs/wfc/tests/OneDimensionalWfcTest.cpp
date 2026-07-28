#include <antwika/wfc/Solver.hpp>

#include <vector>

#include <gtest/gtest.h>

#include <antwika/wfc/AdjacencyConstraint.hpp>
#include <antwika/wfc/CompatibilityTable.hpp>
#include <antwika/wfc/Domain.hpp>
#include <antwika/wfc/IConstraint.hpp>
#include <antwika/wfc/SolveResult.hpp>

using antwika::wfc::AdjacencyConstraint;
using antwika::wfc::CompatibilityTable;
using antwika::wfc::Domain;
using antwika::wfc::SolveOutcome;
using antwika::wfc::Solver;

namespace
{
    // Symbols: 0 = grass, 1 = sand, 2 = water.
    // water only sits next to sand or water; grass never next to water;
    // sand sits next to anything.
    CompatibilityTable makeTable()
    {
        CompatibilityTable table(3);
        table.set(0, 2, false);
        table.set(2, 0, false);
        return table;
    }
} // namespace

TEST(OneDimensionalWfcTest, StripSolvesWithOnlyCompatibleNeighbors)
{
    constexpr std::size_t kLength = 10;
    std::vector<Domain> wave;
    for (std::size_t i = 0; i < kLength; ++i)
    {
        wave.emplace_back(3);
    }

    std::vector<AdjacencyConstraint> constraints;
    constraints.reserve(kLength - 1);
    for (std::size_t i = 0; i + 1 < kLength; ++i)
    {
        constraints.emplace_back(i, i + 1, makeTable());
    }

    std::vector<std::reference_wrapper<const antwika::wfc::IConstraint>>
        constraintRefs(constraints.begin(), constraints.end());

    Solver solver(wave, constraintRefs);
    const auto result = solver.solve();

    ASSERT_EQ(result.outcome, SolveOutcome::Solved);
    ASSERT_EQ(result.assignment.size(), kLength);
    for (std::size_t i = 0; i + 1 < kLength; ++i)
    {
        EXPECT_TRUE(makeTable().compatible(
            result.assignment[i], result.assignment[i + 1]));
    }
}

TEST(OneDimensionalWfcTest, SolvingIsDeterministic)
{
    constexpr std::size_t kLength = 8;
    std::vector<Domain> wave;
    for (std::size_t i = 0; i < kLength; ++i)
    {
        wave.emplace_back(3);
    }

    std::vector<AdjacencyConstraint> constraints;
    constraints.reserve(kLength - 1);
    for (std::size_t i = 0; i + 1 < kLength; ++i)
    {
        constraints.emplace_back(i, i + 1, makeTable());
    }

    std::vector<std::reference_wrapper<const antwika::wfc::IConstraint>>
        constraintRefs(constraints.begin(), constraints.end());

    Solver solverA(wave, constraintRefs);
    Solver solverB(wave, constraintRefs);

    EXPECT_EQ(solverA.solve().assignment, solverB.solve().assignment);
}
