#include <gtest/gtest.h>

#include <string_view>

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
        const auto attempt = solvePuzzle(Board{}, {.maxSteps = 1});

        EXPECT_EQ(attempt.outcome, SolveOutcome::LimitExceeded);
        EXPECT_EQ(attempt.board.format(), std::string(81, '.'));
    }
}
