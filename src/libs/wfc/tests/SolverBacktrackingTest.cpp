#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <vector>

#include <antwika/wfc/Solver.hpp>
#include <antwika/wfc/AdjacencyConstraint.hpp>
#include <antwika/wfc/AllDifferentConstraint.hpp>
#include <antwika/wfc/CompatibilityTable.hpp>
#include <antwika/wfc/Domain.hpp>
#include <antwika/wfc/SolveResult.hpp>
#include <antwika/wfc/mocks/MockConstraint.hpp>

using antwika::wfc::AdjacencyConstraint;
using antwika::wfc::AllDifferentConstraint;
using antwika::wfc::CompatibilityTable;
using antwika::wfc::Domain;
using antwika::wfc::SolveOutcome;
using antwika::wfc::Solver;
using antwika::wfc::mocks::MockConstraint;
using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;

TEST(SolverBacktrackingTest, Solve_AbandonsAWrongBranch)
{
    static const std::vector<std::size_t> cellIndices{0};

    MockConstraint constraint;
    ON_CALL(constraint, cells())
        .WillByDefault(Return(std::span<const std::size_t>(cellIndices)));
    EXPECT_CALL(constraint, cells()).WillRepeatedly(Return(
        std::span<const std::size_t>(cellIndices)));
    EXPECT_CALL(constraint, prune(_))
        .WillRepeatedly(Invoke(
            [](std::vector<Domain> &wave)
            {
                if (wave[0].isSingleton() && wave[0].singleValue() == 0)
                {
                    return false;
                }
                return true;
            }));

    std::vector<Domain> wave{Domain(2)};
    Solver solver(wave, {std::cref(constraint)});
    const auto result = solver.solve();

    EXPECT_EQ(result.outcome, SolveOutcome::Solved);
    EXPECT_EQ(result.assignment, (std::vector<std::size_t>{1}));
}

TEST(SolverBacktrackingTest, Solve_PopsToAnOuterChoicePoint)
{
    static const std::vector<std::size_t> cellIndices{0, 1};

    MockConstraint constraint;
    ON_CALL(constraint, cells())
        .WillByDefault(Return(std::span<const std::size_t>(cellIndices)));
    EXPECT_CALL(constraint, cells()).WillRepeatedly(Return(
        std::span<const std::size_t>(cellIndices)));
    EXPECT_CALL(constraint, prune(_))
        .WillRepeatedly(Invoke(
            [](std::vector<Domain> &wave)
            {
                if (!wave[0].isSingleton())
                {
                    return true;
                }
                const std::size_t v0 = wave[0].singleValue();
                if (!wave[1].isSingleton())
                {
                    return true;
                }
                const std::size_t v1 = wave[1].singleValue();
                if (v0 == 0)
                {
                    return false;
                }
                return v1 == 0;
            }));

    Domain cell0(3);
    cell0.remove(2);
    std::vector<Domain> wave{cell0, Domain(3)};
    Solver solver(wave, {std::cref(constraint)});
    const auto result = solver.solve();

    EXPECT_EQ(result.outcome, SolveOutcome::Solved);
    EXPECT_EQ(result.assignment, (std::vector<std::size_t>{1, 0}));
}

TEST(
    SolverBacktrackingTest,
    Solve_UndoesPartialMutationsOnBacktrack)
{
    std::vector<Domain> wave{Domain(3), Domain(3)};
    wave[1].remove(2);
    AllDifferentConstraint allDifferent({0, 1});

    CompatibilityTable table(3);
    table.set(0, 0, false);
    table.set(0, 1, false);
    table.set(0, 2, true);
    table.set(1, 0, false);
    table.set(1, 1, true);
    table.set(1, 2, false);
    AdjacencyConstraint adjacency(1, 0, table);

    Solver solver(wave, {std::cref(allDifferent), std::cref(adjacency)});
    const auto result = solver.solve();

    EXPECT_EQ(result.outcome, SolveOutcome::Solved);
    EXPECT_EQ(result.assignment, (std::vector<std::size_t>{2, 0}));
}
