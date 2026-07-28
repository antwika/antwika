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
using antwika::wfc::IConstraint;
using antwika::wfc::SolveOutcome;
using antwika::wfc::Solver;

namespace
{
    // Every pair of symbols is compatible: mostly-satisfiable by
    // design, per PLAN_WFC.md 8, so this stresses propagation/trail
    // scale rather than search depth.
    CompatibilityTable makeFullyCompatibleTable()
    {
        return CompatibilityTable(4);
    }
} // namespace

TEST(SolverLargeScaleTest, ThousandsOfCellsCompleteWithValidAssignment)
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
        constraints.emplace_back(
            i, i + 1, makeFullyCompatibleTable());
    }

    std::vector<std::reference_wrapper<const IConstraint>> constraintRefs(
        constraints.begin(), constraints.end());

    Solver solver(wave, constraintRefs);
    const auto result = solver.solve();

    ASSERT_EQ(result.outcome, SolveOutcome::Solved);
    ASSERT_EQ(result.assignment.size(), kLength);
    const CompatibilityTable table = makeFullyCompatibleTable();
    for (std::size_t i = 0; i + 1 < kLength; ++i)
    {
        EXPECT_TRUE(table.compatible(
            result.assignment[i], result.assignment[i + 1]));
    }
}
