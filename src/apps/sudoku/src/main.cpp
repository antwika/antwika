#include <cstddef>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/wfc/AllDifferentConstraint.hpp>
#include <antwika/wfc/IConstraint.hpp>
#include <antwika/wfc/SolveResult.hpp>
#include <antwika/wfc/Solver.hpp>

#include "antwika/sudoku/Board.hpp"
#include "antwika/sudoku/BoardFormatError.hpp"
#include "antwika/sudoku/Puzzle.hpp"

using antwika::sudoku::Board;
using antwika::sudoku::BoardFormatError;
using antwika::sudoku::buildConstraints;
using antwika::sudoku::buildInitialWave;
using antwika::wfc::AllDifferentConstraint;
using antwika::wfc::IConstraint;
using antwika::wfc::SolveOutcome;
using antwika::wfc::Solver;

namespace
{
    // A well-known easy demo puzzle, used when --puzzle is omitted.
    constexpr std::string_view kDemoPuzzle =
        "53..7...."
        "6..195..."
        ".98....6."
        "8...6...3"
        "4..8.3..1"
        "7...2...6"
        ".6....28."
        "...419..5"
        "....8..79";

    Board loadBoard(std::string_view puzzlePath)
    {
        if (puzzlePath.empty())
        {
            return Board::parse(kDemoPuzzle);
        }

        std::ifstream file{std::string(puzzlePath)};

        // Unchecked, a missing file read as an empty puzzle.
        // Which Board::parse then reported as the wrong length.
        if (!file.is_open())
        {
            throw BoardFormatError(
                "antwika_sudoku: could not open a puzzle: "
                + std::string(puzzlePath));
        }

        std::ostringstream contents;
        contents << file.rdbuf();
        return Board::parse(contents.str());
    }

    void printBoard(const Board &board)
    {
        for (std::size_t row = 0; row < Board::kSize; ++row)
        {
            for (std::size_t col = 0; col < Board::kSize; ++col)
            {
                const auto digit = board.at(row, col);
                std::cout << (digit.has_value() ? std::to_string(*digit)
                                                 : ".");
            }
            std::cout << '\n';
        }
    }
} // namespace

int main(int argc, char **argv)
{
    std::string_view puzzlePath;
    for (int i = 1; i < argc; ++i)
    {
        const std::string_view arg = argv[i];
        if (arg == "--puzzle")
        {
            if (i + 1 >= argc)
            {
                std::cerr << "Missing value for --puzzle\n";
                return 1;
            }
            puzzlePath = argv[++i];
        }
        else
        {
            std::cerr << "Unrecognized argument: " << arg << '\n';
            return 1;
        }
    }

    Board board;
    try
    {
        board = loadBoard(puzzlePath);
    }
    catch (const BoardFormatError &error)
    {
        std::cerr << "Invalid puzzle: " << error.what() << '\n';
        return 1;
    }

    std::cout << "Input:\n";
    printBoard(board);

    const auto wave = buildInitialWave(board);
    const auto constraints = buildConstraints();
    std::vector<std::reference_wrapper<const IConstraint>> constraintRefs(
        constraints.begin(), constraints.end());

    const Solver solver(wave, constraintRefs);
    const auto result = solver.solve();

    switch (result.outcome)
    {
        case SolveOutcome::Solved:
        {
            Board solved;
            for (std::size_t row = 0; row < Board::kSize; ++row)
            {
                for (std::size_t col = 0; col < Board::kSize; ++col)
                {
                    const std::size_t index = row * Board::kSize + col;
                    const int digit = static_cast<int>(
                        result.assignment[index]) + 1;
                    solved.set(row, col, digit);
                }
            }
            std::cout << "\nSolved:\n";
            printBoard(solved);
            return 0;
        }
        case SolveOutcome::Unsatisfiable:
            std::cout << "\nNo solution exists for this puzzle.\n";
            return 1;
        case SolveOutcome::LimitExceeded:
            // Cannot occur: no SolverLimits is set above.
            // Handled explicitly anyway, so the switch stays exhaustive.
            std::cout << "\nSolver step limit exceeded.\n";
            return 1;
    }

    return 1;
}
