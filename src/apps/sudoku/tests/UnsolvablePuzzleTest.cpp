#include <gtest/gtest.h>

#include <vector>

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

TEST(UnsolvablePuzzleTest, BuildInitialWave_RefusesContradictoryGivens)
{
    Board board;
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
