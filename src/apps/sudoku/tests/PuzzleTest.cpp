#include <gtest/gtest.h>

#include <algorithm>
#include <vector>

#include <antwika/sudoku/Puzzle.hpp>
#include <antwika/wfc/SolveResult.hpp>
#include <antwika/sudoku/PuzzleFile.hpp>
#include <antwika/sudoku/Solve.hpp>

using antwika::sudoku::Board;
using antwika::sudoku::buildConstraints;
using antwika::sudoku::buildInitialWave;

TEST(PuzzleTest, BuildConstraints_MakesTwentySeven)
{
    const auto constraints = buildConstraints();
    EXPECT_EQ(constraints.size(), 27U);
}

TEST(PuzzleTest, BuildConstraints_CoversTheFirstRow)
{
    const auto constraints = buildConstraints();
    std::vector<std::size_t> cells(
        constraints[0].cells().begin(), constraints[0].cells().end());
    EXPECT_EQ(cells, (std::vector<std::size_t>{0, 1, 2, 3, 4, 5, 6, 7, 8}));
}

TEST(PuzzleTest, BuildConstraints_CoversTheFirstColumn)
{
    const auto constraints = buildConstraints();
    std::vector<std::size_t> cells(
        constraints[9].cells().begin(), constraints[9].cells().end());
    EXPECT_EQ(
        cells,
        (std::vector<std::size_t>{0, 9, 18, 27, 36, 45, 54, 63, 72}));
}

TEST(PuzzleTest, BuildConstraints_CoversTheFirstBox)
{
    const auto constraints = buildConstraints();
    std::vector<std::size_t> cells(
        constraints[18].cells().begin(), constraints[18].cells().end());
    EXPECT_EQ(
        cells,
        (std::vector<std::size_t>{0, 1, 2, 9, 10, 11, 18, 19, 20}));
}

TEST(PuzzleTest, BuildInitialWave_MatchesGivensAndBlanks)
{
    Board board;
    board.set(0, 0, 5);

    const auto wave = buildInitialWave(board);
    ASSERT_EQ(wave.size(), Board::kCellCount);

    EXPECT_TRUE(wave[0].isSingleton());
    EXPECT_EQ(wave[0].singleValue(), 4U);

    EXPECT_FALSE(wave[1].isSingleton());
    EXPECT_EQ(wave[1].count(), Board::kSize);
}

TEST(PuzzleTest, ObeysRules_AcceptsAGridSomebodyIsPartWayThrough)
{
    Board board;
    board.set(0, 0, 5);
    board.set(4, 4, 5);

    EXPECT_TRUE(antwika::sudoku::obeysRules(board));
}

TEST(PuzzleTest, ObeysRules_RefusesOneDigitTwiceInAGroup)
{
    Board row;
    row.set(0, 0, 5);
    row.set(0, 4, 5);
    EXPECT_FALSE(antwika::sudoku::obeysRules(row));

    Board column;
    column.set(0, 0, 5);
    column.set(4, 0, 5);
    EXPECT_FALSE(antwika::sudoku::obeysRules(column));

    Board box;
    box.set(0, 0, 5);
    box.set(1, 1, 5);
    box.set(2, 2, 5);
    EXPECT_FALSE(antwika::sudoku::obeysRules(box));
}

TEST(PuzzleTest, IsComplete_WantsEverySquareFilledAndEveryRuleKept)
{
    const auto solved = antwika::sudoku::solvePuzzle(
        Board::parse(antwika::sudoku::kDemoPuzzle));
    ASSERT_EQ(solved.outcome, antwika::wfc::SolveOutcome::Solved);

    EXPECT_TRUE(antwika::sudoku::isComplete(solved.board));

    Board oneShort = solved.board;
    oneShort.set(4, 4, 0);
    EXPECT_FALSE(antwika::sudoku::isComplete(oneShort));

    Board wrong = solved.board;
    wrong.set(0, 0, wrong.at(0, 1).value());
    EXPECT_FALSE(antwika::sudoku::isComplete(wrong));
}
