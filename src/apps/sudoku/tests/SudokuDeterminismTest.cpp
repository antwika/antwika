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

    constexpr std::string_view kOnlySolution =
        "534678912"
        "672195348"
        "198342567"
        "859761423"
        "426853791"
        "713924856"
        "961537284"
        "287419635"
        "345286179";

    std::vector<std::size_t> zeroBasedDigits(std::string_view grid)
    {
        std::vector<std::size_t> values;
        values.reserve(grid.size());
        for (const char digit : grid)
        {
            values.push_back(static_cast<std::size_t>(digit - '1'));
        }
        return values;
    }
}

TEST(SudokuDeterminismTest, Solve_IsIdenticalForTheSamePuzzle)
{
    const Board board = Board::parse(kPuzzle);
    const auto constraints = buildConstraints();
    std::vector<std::reference_wrapper<const IConstraint>> refs(
        constraints.begin(), constraints.end());

    const Solver solverA(buildInitialWave(board), refs);
    const Solver solverB(buildInitialWave(board), refs);

    const auto resultA = solverA.solve();
    const auto resultB = solverB.solve();

    EXPECT_EQ(resultA.outcome, SolveOutcome::Solved);
    EXPECT_EQ(resultA.assignment, zeroBasedDigits(kOnlySolution));
    EXPECT_EQ(resultB.outcome, SolveOutcome::Solved);
    EXPECT_EQ(resultB.assignment, zeroBasedDigits(kOnlySolution));
}
