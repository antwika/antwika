#include "antwika/sudoku/Board.hpp"

#include <cctype>

#include "antwika/sudoku/BoardFormatError.hpp"

namespace antwika::sudoku
{

    namespace
    {
        std::string stripWhitespace(std::string_view text)
        {
            std::string result;
            result.reserve(text.size());
            for (const char c : text)
            {
                if (!std::isspace(static_cast<unsigned char>(c)))
                {
                    result.push_back(c);
                }
            }
            return result;
        }
    } // namespace

    Board Board::parse(std::string_view text)
    {
        const std::string stripped = stripWhitespace(text);
        if (stripped.size() != kCellCount)
        {
            throw BoardFormatError(
                "Sudoku board must have exactly 81 non-whitespace "
                "characters");
        }

        Board board;
        for (std::size_t i = 0; i < kCellCount; ++i)
        {
            const char c = stripped[i];
            if (c == '.' || c == '0')
            {
                board.cells[i] = 0;
            }
            else if (c >= '1' && c <= '9')
            {
                board.cells[i] = c - '0';
            }
            else
            {
                throw BoardFormatError(
                    "Sudoku board contains an invalid character");
            }
        }
        return board;
    }

    std::string Board::format() const
    {
        std::string result;
        result.reserve(kCellCount);
        for (const int digit : cells)
        {
            result.push_back(digit == 0 ? '.' : static_cast<char>(
                '0' + digit));
        }
        return result;
    }

    std::optional<int> Board::at(std::size_t row, std::size_t col) const
    {
        const int digit = cells[row * kSize + col];
        if (digit == 0)
        {
            return std::nullopt;
        }
        return digit;
    }

    void Board::set(std::size_t row, std::size_t col, int digit)
    {
        cells[row * kSize + col] = digit;
    }

} // namespace antwika::sudoku
