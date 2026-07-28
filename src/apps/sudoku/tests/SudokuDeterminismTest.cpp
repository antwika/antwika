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

namespace
{
    constexpr std::string_view kPuzzle =
        "53..7...."
        "6..195..."
        ".98....6."
        "8...6...3"
        "4..8.3..1"
        "7...2...6"
        ".6....28."
        "...419..5"
        "....8..79";
} // namespace

TEST(SudokuDeterminismTest, SamePuzzleSolvedTwiceMatches)
{
    const Board board = Board::parse(kPuzzle);
    const auto constraints = buildConstraints();
    std::vector<std::reference_wrapper<const IConstraint>> refs(
        constraints.begin(), constraints.end());

    const Solver solverA(buildInitialWave(board), refs);
    const Solver solverB(buildInitialWave(board), refs);

    const auto resultA = solverA.solve();
    const auto resultB = solverB.solve();

    EXPECT_EQ(resultA.outcome, resultB.outcome);
    EXPECT_EQ(resultA.assignment, resultB.assignment);
    EXPECT_EQ(resultA.outcome, SolveOutcome::Solved);
}
