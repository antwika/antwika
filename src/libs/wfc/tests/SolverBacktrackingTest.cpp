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
    ON_CALL(constraint, getCells())
        .WillByDefault(Return(std::span<const std::size_t>(cellIndices)));
    EXPECT_CALL(constraint, getCells()).WillRepeatedly(Return(
        std::span<const std::size_t>(cellIndices)));
    EXPECT_CALL(constraint, prune(_))
        .WillRepeatedly(Invoke(
            [](std::vector<Domain> &waveDomains)
            {
                if (waveDomains[0].isSingleton()
                    && waveDomains[0].getSingleValue() == 0)
                {
                    return false;
                }
                return true;
            }));

    std::vector<Domain> waveDomains{Domain(2)};
    Solver solver(waveDomains, {std::cref(constraint)});
    const auto result = solver.getSolveResult();

    EXPECT_EQ(result.outcome, SolveOutcome::Solved);
    EXPECT_EQ(result.assignment, (std::vector<std::size_t>{1}));
}

TEST(SolverBacktrackingTest, Solve_PopsToAnOuterChoicePoint)
{
    static const std::vector<std::size_t> cellIndices{0, 1};

    MockConstraint constraint;
    ON_CALL(constraint, getCells())
        .WillByDefault(Return(std::span<const std::size_t>(cellIndices)));
    EXPECT_CALL(constraint, getCells()).WillRepeatedly(Return(
        std::span<const std::size_t>(cellIndices)));
    EXPECT_CALL(constraint, prune(_))
        .WillRepeatedly(Invoke(
            [](std::vector<Domain> &waveDomains)
            {
                if (!waveDomains[0].isSingleton())
                {
                    return true;
                }
                const std::size_t v0 = waveDomains[0].getSingleValue();
                if (!waveDomains[1].isSingleton())
                {
                    return true;
                }
                const std::size_t v1 = waveDomains[1].getSingleValue();
                if (v0 == 0)
                {
                    return false;
                }
                return v1 == 0;
            }));

    Domain cell0Domain(3);
    cell0Domain.remove(2);
    std::vector<Domain> waveDomains{cell0Domain, Domain(3)};
    Solver solver(waveDomains, {std::cref(constraint)});
    const auto result = solver.getSolveResult();

    EXPECT_EQ(result.outcome, SolveOutcome::Solved);
    EXPECT_EQ(result.assignment, (std::vector<std::size_t>{1, 0}));
}

TEST(
    SolverBacktrackingTest,
    Solve_UndoesPartialMutationsOnBacktrack)
{
    std::vector<Domain> waveDomains{Domain(3), Domain(3)};
    waveDomains[1].remove(2);
    AllDifferentConstraint allDifferent({0, 1});

    CompatibilityTable table(3);
    table.set(0, 0, false);
    table.set(0, 1, false);
    table.set(0, 2, true);
    table.set(1, 0, false);
    table.set(1, 1, true);
    table.set(1, 2, false);
    AdjacencyConstraint adjacency(1, 0, table);

    Solver solver(waveDomains, {std::cref(allDifferent), std::cref(adjacency)});
    const auto result = solver.getSolveResult();

    EXPECT_EQ(result.outcome, SolveOutcome::Solved);
    EXPECT_EQ(result.assignment, (std::vector<std::size_t>{2, 0}));
}
