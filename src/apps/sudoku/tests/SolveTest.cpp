#include <string_view>

#include <gtest/gtest.h>

#include <antwika/wfc/SolveResult.hpp>

#include <antwika/sudoku/Board.hpp>
#include <antwika/sudoku/PuzzleFile.hpp>
#include <antwika/sudoku/Solve.hpp>

using antwika::sudoku::Board;
using antwika::sudoku::kDemoPuzzle;
using antwika::sudoku::solvePuzzle;
using antwika::wfc::SolveOutcome;

namespace
{
    constexpr std::string_view kDemoSolution =
        "534678912"
        "672195348"
        "198342567"
        "859761423"
        "426853791"
        "713924856"
        "961537284"
        "287419635"
        "345286179";

    TEST(SolveTest, SolvePuzzle_FinishesTheDemoGrid)
    {
        const auto attempt = solvePuzzle(Board::parse(kDemoPuzzle));

        EXPECT_EQ(attempt.outcome, SolveOutcome::Solved);
        EXPECT_EQ(attempt.board.format(), kDemoSolution);
    }

    TEST(SolveTest, SolvePuzzle_ContinuesFromWhatSomebodyHasTyped)
    {
        Board board = Board::parse(kDemoPuzzle);

        // The first blank, filled with what the solution says.
        board.set(0, 2, 4);

        const auto attempt = solvePuzzle(board);

        EXPECT_EQ(attempt.outcome, SolveOutcome::Solved);
        EXPECT_EQ(attempt.board.format(), kDemoSolution);
    }

    TEST(SolveTest, SolvePuzzle_HandsBackNoGridWhenThereIsNone)
    {
        Board board;
        board.set(0, 0, 5);
        board.set(0, 1, 5);

        const auto attempt = solvePuzzle(board);

        EXPECT_EQ(attempt.outcome, SolveOutcome::Unsatisfiable);
        EXPECT_EQ(attempt.board.format(), std::string(81, '.'));
    }

    TEST(SolveTest, SolvePuzzle_ReportsRunningOutOfTheStepsAllowed)
    {
        // An empty grid is all search and no propagation.
        // So a budget of one step cannot get to the end of it.
        const auto attempt = solvePuzzle(Board{}, {.maxSteps = 1});

        EXPECT_EQ(attempt.outcome, SolveOutcome::LimitExceeded);
        EXPECT_EQ(attempt.board.format(), std::string(81, '.'));
    }

    TEST(SolveTest, SolvePuzzle_SolvesTheSameGridTheSameWayTwice)
    {
        const Board board = Board::parse(kDemoPuzzle);

        EXPECT_EQ(
            solvePuzzle(board).board.format(),
            solvePuzzle(board).board.format());
    }
} // namespace
