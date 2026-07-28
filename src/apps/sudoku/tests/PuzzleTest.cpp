#include <antwika/sudoku/Puzzle.hpp>

#include <algorithm>
#include <vector>

#include <gtest/gtest.h>

using antwika::sudoku::Board;
using antwika::sudoku::buildConstraints;
using antwika::sudoku::buildInitialWave;

TEST(PuzzleTest, ConstraintCountIsTwentySeven)
{
    const auto constraints = buildConstraints();
    EXPECT_EQ(constraints.size(), 27U);
}

TEST(PuzzleTest, FirstRowCoversExpectedIndices)
{
    const auto constraints = buildConstraints();
    std::vector<std::size_t> cells(
        constraints[0].cells().begin(), constraints[0].cells().end());
    EXPECT_EQ(cells, (std::vector<std::size_t>{0, 1, 2, 3, 4, 5, 6, 7, 8}));
}

TEST(PuzzleTest, FirstColumnCoversExpectedIndices)
{
    const auto constraints = buildConstraints();
    // Constraints[9] is the first column constraint (9 rows first).
    std::vector<std::size_t> cells(
        constraints[9].cells().begin(), constraints[9].cells().end());
    EXPECT_EQ(
        cells,
        (std::vector<std::size_t>{0, 9, 18, 27, 36, 45, 54, 63, 72}));
}

TEST(PuzzleTest, FirstBoxCoversExpectedIndices)
{
    const auto constraints = buildConstraints();
    // Constraints[18] is the first box constraint (9 rows + 9 cols).
    std::vector<std::size_t> cells(
        constraints[18].cells().begin(), constraints[18].cells().end());
    EXPECT_EQ(
        cells,
        (std::vector<std::size_t>{0, 1, 2, 9, 10, 11, 18, 19, 20}));
}

TEST(PuzzleTest, InitialWaveMatchesGivensAndBlanks)
{
    Board board;
    board.set(0, 0, 5);

    const auto wave = buildInitialWave(board);
    ASSERT_EQ(wave.size(), Board::kCellCount);

    EXPECT_TRUE(wave[0].isSingleton());
    EXPECT_EQ(wave[0].singleValue(), 4U); // digit 5 -> index 4

    EXPECT_FALSE(wave[1].isSingleton());
    EXPECT_EQ(wave[1].count(), Board::kSize);
}
