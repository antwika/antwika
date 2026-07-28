#include <antwika/wfc/Solver.hpp>

#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/wfc/Domain.hpp>
#include <antwika/wfc/SolveResult.hpp>
#include <antwika/wfc/mocks/MockConstraint.hpp>

using antwika::wfc::Domain;
using antwika::wfc::SolveOutcome;
using antwika::wfc::Solver;
using antwika::wfc::mocks::MockConstraint;
using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;

TEST(SolverBacktrackingTest, WrongBranchIsAbandonedBeforeCorrectOneFound)
{
    // A single cell with two candidates. Candidate 0 (tried first, per
    // ascending order) is a manufactured dead end; only candidate 1
    // actually satisfies the constraint. Propagation alone can't know
    // this in advance -- it only surfaces once the branch is tried.
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

TEST(SolverBacktrackingTest, ExhaustingAnInnerChoicePointPopsToAnOuterOne)
{
    // Two choice points deep: cell 0 (2 candidates) is chosen first
    // (fewer candidates -> lower entropy). Its first candidate (0)
    // leaves cell 1 (3 candidates) as a second, inner choice point --
    // every one of *its* candidates is a dead end while cell 0 == 0,
    // so the inner choice point is fully exhausted and popped while
    // the outer one (cell 0) is still on the stack, forcing a rewind
    // to it rather than reporting Unsatisfiable. Only cell 0 == 1
    // paired with cell 1 == 0 actually satisfies the constraint.
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
