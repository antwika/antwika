#include <vector>

#include <gtest/gtest.h>

#include <antwika/wfc/IConstraint.hpp>
#include <antwika/wfc/SolveResult.hpp>
#include <antwika/wfc/Solver.hpp>

#include <antwika/sudoku/Board.hpp>
#include <antwika/sudoku/Puzzle.hpp>

using antwika::sudoku::Board;
using antwika::sudoku::buildConstraints;
using antwika::sudoku::buildInitialWave;
using antwika::wfc::IConstraint;
using antwika::wfc::SolveOutcome;
using antwika::wfc::Solver;

TEST(UnsolvablePuzzleTest, ContradictoryGivensAreUnsatisfiable)
{
    Board board;
    // Two 5s in the same row -- immediately contradictory.
    board.set(0, 0, 5);
    board.set(0, 1, 5);

    const auto wave = buildInitialWave(board);
    const auto constraints = buildConstraints();
    std::vector<std::reference_wrapper<const IConstraint>> refs(
        constraints.begin(), constraints.end());

    const Solver solver(wave, refs);
    const auto result = solver.solve();

    EXPECT_EQ(result.outcome, SolveOutcome::Unsatisfiable);
    EXPECT_TRUE(result.assignment.empty());
}
