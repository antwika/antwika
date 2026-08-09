#include <gtest/gtest.h>

#include <algorithm>
#include <array>
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
    constexpr std::string_view kEasyPuzzle =
        "53..7...."
        "6..195..."
        ".98....6."
        "8...6...3"
        "4..8.3..1"
        "7...2...6"
        ".6....28."
        "...419..5"
        "....8..79";

    constexpr std::string_view kEasySolution =
        "534678912"
        "672195348"
        "198342567"
        "859761423"
        "426853791"
        "713924856"
        "961537284"
        "287419635"
        "345286179";

    constexpr std::string_view kHardPuzzle =
        "53......."
        "6..1....."
        ".98....6."
        "8...6...."
        "4..8....."
        "7...2...."
        ".6....28."
        "...4....."
        ".........";

    bool isPermutationOfOneToNine(std::vector<int> values)
    {
        std::sort(values.begin(), values.end());
        return values == std::vector<int>{1, 2, 3, 4, 5, 6, 7, 8, 9};
    }

    bool isValidSolution(const Board &board)
    {
        for (std::size_t r = 0; r < Board::kSize; ++r)
        {
            std::vector<int> row;
            for (std::size_t c = 0; c < Board::kSize; ++c)
            {
                const auto v = board.at(r, c);
                if (!v.has_value())
                {
                    return false;
                }
                row.push_back(*v);
            }
            if (!isPermutationOfOneToNine(row))
            {
                return false;
            }
        }
        for (std::size_t c = 0; c < Board::kSize; ++c)
        {
            std::vector<int> col;
            for (std::size_t r = 0; r < Board::kSize; ++r)
            {
                col.push_back(*board.at(r, c));
            }
            if (!isPermutationOfOneToNine(col))
            {
                return false;
            }
        }
        for (std::size_t b = 0; b < Board::kSize; ++b)
        {
            std::vector<int> box;
            for (std::size_t dr = 0; dr < 3; ++dr)
            {
                for (std::size_t dc = 0; dc < 3; ++dc)
                {
                    const std::size_t row = 3 * (b / 3) + dr;
                    const std::size_t col = 3 * (b % 3) + dc;
                    box.push_back(*board.at(row, col));
                }
            }
            if (!isPermutationOfOneToNine(box))
            {
                return false;
            }
        }
        return true;
    }

    Board solve(std::string_view puzzleText)
    {
        const Board board = Board::parse(puzzleText);
        const auto wave = buildInitialWave(board);
        const auto constraints = buildConstraints();
        std::vector<std::reference_wrapper<const IConstraint>> refs(
            constraints.begin(), constraints.end());

        const Solver solver(wave, refs);
        const auto result = solver.solve();
        EXPECT_EQ(result.outcome, SolveOutcome::Solved);

        Board solved;
        for (std::size_t row = 0; row < Board::kSize; ++row)
        {
            for (std::size_t col = 0; col < Board::kSize; ++col)
            {
                const std::size_t index = row * Board::kSize + col;
                solved.set(
                    row,
                    col,
                    static_cast<int>(result.assignment[index]) + 1);
            }
        }
        return solved;
    }
}

TEST(SudokuSolverIntegrationTest, Solve_MatchesAKnownSolution)
{
    const Board solved = solve(kEasyPuzzle);
    EXPECT_EQ(solved.format(), kEasySolution);
}

TEST(SudokuSolverIntegrationTest, Solve_ProducesAValidSolutionWhenHarder)
{
    const Board solved = solve(kHardPuzzle);
    EXPECT_TRUE(isValidSolution(solved));

    const Board given = Board::parse(kHardPuzzle);
    for (std::size_t row = 0; row < Board::kSize; ++row)
    {
        for (std::size_t col = 0; col < Board::kSize; ++col)
        {
            const auto g = given.at(row, col);
            if (g.has_value())
            {
                EXPECT_EQ(solved.at(row, col), g);
            }
        }
    }
}
