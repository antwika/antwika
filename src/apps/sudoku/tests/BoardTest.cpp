#include <gtest/gtest.h>

#include <antwika/sudoku/Board.hpp>
#include <antwika/sudoku/BoardFormatError.hpp>

using antwika::sudoku::Board;
using antwika::sudoku::BoardFormatError;

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
}

TEST(BoardTest, Parse_RoundTripsThroughFormat)
{
    const Board board = Board::parse(kPuzzle);
    EXPECT_EQ(board.format(), kPuzzle);
}

TEST(BoardTest, At_ReturnsGivenDigitsAndBlanks)
{
    const Board board = Board::parse(kPuzzle);
    EXPECT_EQ(board.at(0, 0), 5);
    EXPECT_EQ(board.at(0, 1), 3);
    EXPECT_FALSE(board.at(0, 2).has_value());
}

TEST(BoardTest, Parse_AcceptsZeroAsTheBlankMarker)
{
    const std::string zeros(81, '0');
    const Board board = Board::parse(zeros);
    for (std::size_t row = 0; row < Board::kSize; ++row)
    {
        for (std::size_t col = 0; col < Board::kSize; ++col)
        {
            EXPECT_FALSE(board.at(row, col).has_value());
        }
    }
}

TEST(BoardTest, Parse_StripsWhitespaceAndNewlines)
{
    std::string spaced;
    for (const char c : kPuzzle)
    {
        spaced.push_back(c);
        spaced.push_back('\n');
    }
    const Board board = Board::parse(spaced);
    EXPECT_EQ(board.format(), kPuzzle);
}

TEST(BoardTest, Parse_ThrowsOnTheWrongLength)
{
    EXPECT_THROW(
        { [[maybe_unused]] auto b = Board::parse("53..7...."); },
        BoardFormatError);
}

TEST(BoardTest, Parse_ThrowsOnAnInvalidCharacter)
{
    std::string bad(kPuzzle);
    bad[0] = 'x';
    EXPECT_THROW(
        { [[maybe_unused]] auto b = Board::parse(bad); },
        BoardFormatError);
}

TEST(BoardTest, Parse_ThrowsBelowTheDigitRange)
{
    std::string bad(kPuzzle);
    bad[0] = '#';
    EXPECT_THROW(
        { [[maybe_unused]] auto b = Board::parse(bad); },
        BoardFormatError);
}

TEST(BoardTest, Set_WritesACell)
{
    Board board;
    board.set(4, 4, 7);
    EXPECT_EQ(board.at(4, 4), 7);
}

TEST(BoardTest, At_ThrowsOnAnOutOfRangeRow)
{
    const Board board = Board::parse(kPuzzle);
    EXPECT_THROW(
        { [[maybe_unused]] auto d = board.at(9, 0); }, BoardFormatError);
}

TEST(BoardTest, At_ThrowsOnAnOutOfRangeColumn)
{
    const Board board = Board::parse(kPuzzle);
    EXPECT_THROW(
        { [[maybe_unused]] auto d = board.at(0, 9); }, BoardFormatError);
}

TEST(BoardTest, Set_ThrowsOnAnOutOfRangeRow)
{
    Board board;
    EXPECT_THROW(board.set(9, 0, 5), BoardFormatError);
}

TEST(BoardTest, Set_ThrowsOnAnOutOfRangeColumn)
{
    Board board;
    EXPECT_THROW(board.set(0, 9, 5), BoardFormatError);
}

TEST(BoardTest, Set_ThrowsOnADigitAboveNine)
{
    Board board;
    EXPECT_THROW(board.set(0, 0, 10), BoardFormatError);
}

TEST(BoardTest, Set_ThrowsOnANegativeDigit)
{
    Board board;
    EXPECT_THROW(board.set(0, 0, -1), BoardFormatError);
}
